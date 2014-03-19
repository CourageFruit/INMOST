#include "inmost.h"
#include <stack>
#if defined(USE_MESH)

//TODO:
// incident_matrix class should measure for minimal volume,
// possibly check and update from projects/OctreeCutcell/octgrid.cpp

namespace INMOST
{
	
	void Face::SwapCells()
	{
		MIDType hm = GetMeshLink()->HideMarker();
		if( Mesh::Count(&high_conn[0],high_conn.size(),hm) == 2 )
		{
			INMOST_DATA_ENUM_TYPE k1 = static_cast<INMOST_DATA_ENUM_TYPE>(-1), k2;
			k1 = Mesh::getNext(&high_conn[0],high_conn.size(),k1,hm);
			k2 = Mesh::getNext(&high_conn[0],high_conn.size(),k1,hm);
			Element * temp = high_conn[k1];
			high_conn[k1] = high_conn[k2];
			high_conn[k2] = temp;
			FixNormalOrientation(); //maybe should change orientation?
		}
	}
	
	void Element::Disconnect(bool del_upper)
	{
		INMOST_DATA_ENUM_TYPE i = 0;
		dynarray<INMOST_DATA_ENUM_TYPE,64> del;
		
		//BEGIN NEW CODE - CHECK
		//Reorder face edges, so that they always apear in right direction
		if( GetElementType() == CELL ) //This is a cell
		{
			Mesh * m = GetMeshLink();
			MIDType hm = m->HideMarker();
			for( adj_iterator it = low_conn.begin(); it != low_conn.end(); it++) //iterator over faces
			{
				if( !(*it)->high_conn.empty() && Mesh::Count(&(*it)->high_conn[0],(*it)->high_conn.size(),hm) == 2 ) //there are two cells connected to face
				{
					INMOST_DATA_ENUM_TYPE k1 = static_cast<INMOST_DATA_ENUM_TYPE>(-1), k2;
					k1 = Mesh::getNext(&(*it)->high_conn[0],(*it)->high_conn.size(),k1,hm);
					if( (*it)->high_conn[k1] == this ) //the first cell is current
					{
						k2 = Mesh::getNext(&(*it)->high_conn[0],(*it)->high_conn.size(),k1,hm);
						(*it)->high_conn[k1] = (*it)->high_conn[k2];
						//(*it)->high_conn[k2] = this; //cannot use the cell because virtualization table is already destroyed and FixNormalOrientation will do bad things
						(*it)->high_conn.resize(1); //just remove element, we will do this anyway later
						(*it)->getAsFace()->FixNormalOrientation(); //restore orientation
					}
				}
			}
		}
		//END NEW CODE - CHECK
		
		for( adj_iterator it = high_conn.begin(); it != high_conn.end(); it++)
		{
			int flag = 0;
			adj_iterator jt = (*it)->low_conn.begin();
			while (jt != (*it)->low_conn.end())
			{
				if ((*jt) == this)
				{
					flag = 1;
					jt = (*it)->low_conn.erase(jt);
				}
				else ++jt;
			}
			/*
			for( adj_iterator jt = (*it)->low_conn.begin(); jt != (*it)->low_conn.end(); jt++)
				if( (*jt) == this ) 
				{
					flag = 1;
					jt = (*it)->low_conn.erase(jt);
					if( jt == (*it)->low_conn.end() ) break;
					--jt; 
				}
				*/
			if( flag ) del.push_back(i);
			i++;
		}
		if( del_upper )
		{
			if( GetElementType() < CELL ) 
			{
				if( Hidden() ) for(dynarray<INMOST_DATA_ENUM_TYPE,64>::iterator it = del.begin(); it != del.end(); it++)
					if( high_conn[*it]->Hidden() ) delete high_conn[*it];
				else for(dynarray<INMOST_DATA_ENUM_TYPE,64>::iterator it = del.begin(); it != del.end(); it++)
					delete high_conn[*it];
			}
		}
		for( adj_iterator it = low_conn.begin(); it != low_conn.end(); it++)
		{
			adj_iterator jt = (*it)->high_conn.begin();
			while (jt != (*it)->high_conn.end())
			{
				if ((*jt) == this) jt = (*it)->high_conn.erase(jt);
				else ++jt;
			}
				/*
			for( adj_iterator jt = (*it)->high_conn.begin(); jt != (*it)->high_conn.end(); jt++)
				if( (*jt) == this ) 
				{
					jt = (*it)->high_conn.erase(jt);
					if( jt == (*it)->high_conn.end() ) break;
					--jt;
				}
				*/
		}
	}
	
	Cell * Cell::UniteCells(Cell ** unite, INMOST_DATA_ENUM_TYPE nunite, MIDType del_protect)
	{
		//~ std::cout << "hello there! " << __FUNCTION__ << std::endl;
		
		if( nunite == 0 ) return NULL;
		Mesh * m = unite[0]->GetMeshLink();
		tiny_map<Face *, int, 64> face_visit; // we check all edges of faces, inner edges are visited several times, outer - once
		dynarray<Face *,64> faces, inner_faces;
		bool doexit = false;
		MIDType hm = m->HideMarker();
		for(unsigned j = 0; j < nunite; j++)
		{
			if( unite[j]->GetMarker(del_protect) ) doexit = true; 
			for(adj_iterator it = unite[j]->low_conn.begin(); it != unite[j]->low_conn.end(); it++) if( !(*it)->GetMarker(hm) )
				face_visit[(*it)->getAsFace()]++;
		}
		
		if( doexit ) return NULL;
		
		MIDType visited = m->CreateMarker(), rem = m->CreateMarker();

		dynarray<Edge *,64> edges;
		dynarray<Node *,64> nodes;

					
		for(tiny_map<Face *,int,64>::iterator it = face_visit.begin(); it != face_visit.end(); it++)
			if( it->second == 1 )
				faces.push_back(it->first);
			else
			{
				if( it->first->GetMarker(del_protect) ) doexit = true;
				it->first->SetMarker(rem);
				for(adj_iterator jt = it->first->low_conn.begin(); jt != it->first->low_conn.end(); jt++) if( !(*jt)->GetMarker(hm) )
					if( !(*jt)->GetMarker(visited) )
					{
						(*jt)->SetMarker(visited);
						edges.push_back(static_cast<Edge *>(*jt));
					}
				inner_faces.push_back(it->first);
			}
		
		for(unsigned i = 0; i < edges.size(); i++) 
		{
			edges[i]->RemMarker(visited);
			for(adj_iterator jt = edges[i]->low_conn.begin(); jt != edges[i]->low_conn.end(); jt++) if( !(*jt)->GetMarker(hm) )
				if( !(*jt)->GetMarker(visited) )
				{
					(*jt)->SetMarker(visited);
					nodes.push_back(static_cast<Node *>(*jt));
				}
			
			{
				int nonzero = 0;
				for(adj_iterator jt = edges[i]->high_conn.begin(); jt != edges[i]->high_conn.end(); jt++) if( !(*jt)->GetMarker(hm) )//iterate over faces
					if( !(*jt)->GetMarker(rem) ) nonzero++; // check if it is not deleted
				if( nonzero == 0 ) //all faces should be deleted, edge to remove
				{
					edges[i]->SetMarker(rem);
					if( edges[i]->GetMarker(del_protect) ) doexit = true;
				}
			}
		}
		for(unsigned i = 0; i < nodes.size(); i++) 
		{
			nodes[i]->RemMarker(visited);
			
			if( nodes[i]->GetMarker(del_protect) )
			{
				int nonzero = 0;
				for(adj_iterator jt = nodes[i]->high_conn.begin(); jt != nodes[i]->high_conn.end(); jt++) if( !(*jt)->GetMarker(hm) )
					if( !(*jt)->GetMarker(rem) ) nonzero++;
				if( nonzero == 0 )
					doexit = true;
				
			}
		}
		
		m->ReleaseMarker(visited);
		
		for(dynarray<Face *,64>::iterator it = inner_faces.begin(); it != inner_faces.end(); it++)
			(*it)->RemMarker(rem);
		for(unsigned i = 0; i < edges.size(); i++)  edges[i]->RemMarker(rem);
		m->ReleaseMarker(rem);
		
		if( doexit ) return NULL;
		
		
		
		
		
		//delete cells
		for(unsigned j = 0; j < nunite; j++)
		{
			if( unite[j]->GetMarker(del_protect) )
				std::cout << __FUNCTION__ << " deleted protected cells, united " << nunite << " cells " << std::endl;
			unite[j]->Delete();
		}
		//delete inner faces
		for(unsigned j = 0; j < inner_faces.size(); j++)
		{
			if( inner_faces[j]->GetMarker(del_protect) )
				std::cout << __FUNCTION__ << " deleted protected faces, united " << nunite << " cells " << std::endl;
			inner_faces[j]->Delete();
		}
		for(unsigned j = 0; j < edges.size(); j++) //delete unused edges
			if( edges[j]->high_conn.empty() || Mesh::Count(&edges[j]->high_conn[0],edges[j]->high_conn.size(),hm) == 0 ) //there are no faces that use this edge
			{
				if( edges[j]->GetMarker(del_protect) )
					std::cout << __FUNCTION__ << " deleted protected edge, united " << nunite << " cells " << std::endl;
				edges[j]->Delete();
			}
		for(unsigned j = 0; j < nodes.size(); j++) //delete unused nodes
			if( nodes[j]->high_conn.empty() || Mesh::Count(&nodes[j]->high_conn[0],nodes[j]->high_conn.size(),hm) == 0 ) //there are no edges that use this edge
			{
				if( !nodes[j]->low_conn.empty() && Mesh::Count(&nodes[j]->low_conn[0],nodes[j]->low_conn.size(),hm) != 0 ) throw Impossible; // there must be no cells that use this node
				if( nodes[j]->GetMarker(del_protect) )
					std::cout << __FUNCTION__ << " deleted protected node, united " << nunite << " cells " << std::endl;
				nodes[j]->Delete();
				//disconnect node from cell
				//we don't need this algorithm, because all dependent cells should be deleted
				//if( !nodes[j]->Hide() )
				//{					
					//adj_iterator it = nodes[j]->low_conn.begin();
					//while(it != nodes[j]->low_conn.end()) if( !(*it)->GetMarker(hm) ) //iterate through cells of the node
					//{
					//	adj_iterator jt =(*it)->high_conn.begin();
					//	while(jt != (*it)->high_conn.end() ) if( !(*jt)->GetMarker(hm) ) // iterate through nodes of the cell
					//	{
					//		if( *jt == nodes[j] )
					//			jt = (*it)->high_conn.erase(jt); //erase link to node
					//		else ++jt;
					//	}
					//  ++it;
					//}
					//nodes[j]->Disconnect();
					//nodes[j]->Delete();
				//}
			}
		//reconstruct cell by outer faces
		
		Cell * new_cell = m->CreateCell(&faces[0], faces.size()).first;
		
		
		//~ if( m->HaveGeometricData(ORIENTATION,FACE) )
			//~ for(dynarray<Face *,64>::iterator it = faces.begin(); it != faces.end(); ++it)
				//~ if( (*it)->FixNormalOrientation() ) std::cout << "normal orientation fixed! " << std::endl;
				
		
		return new_cell;
	}
	
	
	bool Cell::TestUniteCells(Cell ** unite, INMOST_DATA_ENUM_TYPE nunite, MIDType del_protect)
	{
		if( nunite == 0 ) return false;
		Mesh * m = unite[0]->GetMeshLink();
		tiny_map<Face *,int,64> face_visit; // we check all edges of faces, inner edges are visited several times, outer - once
		bool doexit = false;
		MIDType hm = m->HideMarker();
					
		for(unsigned j = 0; j < nunite; j++)
		{
			if( unite[j]->GetMarker(del_protect) ) doexit = true; 
			for(adj_iterator it = unite[j]->low_conn.begin(); it != unite[j]->low_conn.end(); it++) if( !(*it)->GetMarker(hm) )
				face_visit[(*it)->getAsFace()]++;
		}
		
		if( doexit ) return false;
		
		MIDType rem = m->CreateMarker();

		dynarray<Edge *,64> edges;
		dynarray<Node *,64> nodes;

					
		for(tiny_map<Face *,int,64>::iterator it = face_visit.begin(); it != face_visit.end(); it++)
			if( it->second != 1 )
			{
				if( it->first->GetMarker(del_protect) ) doexit = true;
				it->first->SetMarker(rem);
				for(adj_iterator jt = it->first->low_conn.begin(); jt != it->first->low_conn.end(); jt++) if( !(*jt)->GetMarker(hm) )
					if( !(*jt)->GetMarker(rem) )
					{
						(*jt)->SetMarker(rem);
						edges.push_back(static_cast<Edge *>(*jt));
					}
			}
		
		for(unsigned i = 0; i < edges.size(); i++) 
		{
			edges[i]->RemMarker(rem);
			for(adj_iterator jt = edges[i]->low_conn.begin(); jt != edges[i]->low_conn.end(); jt++) if( !(*jt)->GetMarker(hm) )
				if( !(*jt)->GetMarker(rem) )
				{
					(*jt)->SetMarker(rem);
					nodes.push_back(static_cast<Node *>(*jt));
				}
			
			{
				int nonzero = 0;
				for(adj_iterator jt = edges[i]->high_conn.begin(); jt != edges[i]->high_conn.end(); jt++) if( !(*jt)->GetMarker(hm) ) //iterate over faces
					if( !(*jt)->GetMarker(rem) ) nonzero++; // check if it is not deleted
				if( nonzero == 0 ) //all faces should be deleted, edge to remove
				{
					edges[i]->SetMarker(rem);
					if( edges[i]->GetMarker(del_protect) ) doexit = true;
				}
			}
		}
		for(unsigned i = 0; i < nodes.size(); i++) 
		{
			nodes[i]->RemMarker(rem);
			
			if( nodes[i]->GetMarker(del_protect) )
			{
				int nonzero = 0;
				for(adj_iterator jt = nodes[i]->high_conn.begin(); jt != nodes[i]->high_conn.end(); jt++) if( !(*jt)->GetMarker(hm) )
					if( !(*jt)->GetMarker(rem) ) nonzero++;
				if( nonzero == 0 )
					doexit = true;
				
			}
		}
		
		
		for(tiny_map<Face *,int,64>::iterator it = face_visit.begin(); it != face_visit.end(); it++)
				if( it->second != 1 ) it->first->RemMarker(rem);
		for(unsigned i = 0; i < edges.size(); i++)  edges[i]->RemMarker(rem);
		m->ReleaseMarker(rem);
		
		if( doexit ) return false;
		return true;
	}
	
	
	Face * Face::UniteFaces(Face ** unite, INMOST_DATA_ENUM_TYPE nunite, MIDType del_protect)
	{
		//~ std::cout << "hello there! " << __FUNCTION__ << std::endl;
		if( nunite == 0 ) return NULL;
		Mesh * m = unite[0]->GetMeshLink();
		MIDType hm = m->HideMarker();
		bool doexit = false, dothrow = false;
		for(unsigned j = 0; j < nunite; j++)
			if( unite[j]->GetMarker(del_protect) ) doexit = true;
			
		if( doexit ) return NULL;
		
		dynarray<Cell *,64> high_conn_set;
		MIDType edge_set = m->CreateMarker();
		MIDType rem = m->CreateMarker();
		
		
		for(unsigned j = 0; j < nunite; j++)
			for(adj_type::iterator it = unite[j]->high_conn.begin(); it != unite[j]->high_conn.end(); it++) if( !(*it)->GetMarker(hm) )
				if( !(*it)->GetMarker(edge_set) )
				{
					high_conn_set.push_back(static_cast<Cell *>(*it));
					(*it)->SetMarker(edge_set);
				}
				
		
		
		for(unsigned j = 0; j < high_conn_set.size(); j++)
			high_conn_set[j]->RemMarker(edge_set);
		
		
		if( m->GetTopologyCheck(TRIPLE_SHARED_FACE) && high_conn_set.size() > 2 )
		{
			m->SetTopologyError(TRIPLE_SHARED_FACE);
			if( m->GetTopologyCheck(PRINT_NOTIFY) ) std::cerr << TopologyCheckNotifyString(TRIPLE_SHARED_FACE) << std::endl;
			if( m->GetTopologyCheck(THROW_EXCEPTION) ) throw TopologyCheckError;
		}
		
		dynarray<Node *,64> nodes;
		tiny_map<Edge *, int,64> edge_visit;
		dynarray<Edge *,64> edges;
		for(unsigned j = 0; j < nunite; j++)
		{
			for(adj_iterator it = unite[j]->low_conn.begin(); it != unite[j]->low_conn.end(); it++) if( !(*it)->GetMarker(hm) )
				edge_visit[(*it)->getAsEdge()]++;
		}
		
		
		
		
		
		
		Edge * first = NULL;
		Node * prev = NULL;
		
		for(tiny_map<Edge *,int,64>::iterator it = edge_visit.begin(); it != edge_visit.end(); it++)
		{
			if( it->second == 1 )
			{
				it->first->SetMarker(edge_set);
				if( first == NULL ) first = it->first;
			}
			else if( it->second == 2 )
			{
				if( it->first->GetMarker(del_protect) ) doexit = true; // edge is protected
				it->first->SetMarker(rem);
				for(adj_iterator jt = it->first->low_conn.begin(); jt != it->first->low_conn.end(); jt++) if( !(*jt)->GetMarker(hm) )
					if( !(*jt)->GetMarker(edge_set) )
					{
						(*jt)->SetMarker(edge_set);
						nodes.push_back(static_cast<Node *>(*jt));
					}
			}
			else 
			{
				doexit = true; //the faces we unite would not appear as one surface
				dothrow = true;
			}
		}
		
		for(unsigned j = 0; j < nodes.size(); j++) 
		{
			nodes[j]->RemMarker(edge_set);
			if( nodes[j]->GetMarker(del_protect) )
			{
				int nonzero = 0;
				for(adj_iterator it = nodes[j]->high_conn.begin(); it != nodes[j]->high_conn.end(); it++) if( !(*it)->GetMarker(hm) ) //iterate through edges of the node
					if( !(*it)->GetMarker(rem) ) nonzero++; // check if edge should not be deleted
				if( nonzero == 0 ) //all edges are deleted but the node is protected
					doexit = true;
			}
		}
		
		
			
		if( doexit )
		{
			for(tiny_map<Edge *,int,64>::iterator it = edge_visit.begin(); it != edge_visit.end(); it++) 
				if( it->second != 1 ) it->first->RemMarker(rem);
				else it->first->RemMarker(edge_set);
				
			m->ReleaseMarker(edge_set);
			m->ReleaseMarker(rem);
			if( dothrow ) throw Impossible; //report the situation, because user need to debug the input
			return NULL;
		}

		edges.push_back(first);
		edges.back()->RemMarker(edge_set);
		bool done = false;
		int q = 0;
		while( !done )
		{
			INMOST_DATA_ENUM_TYPE k1 = static_cast<INMOST_DATA_ENUM_TYPE>(-1),k2;
			k1 = Mesh::getNext(&edges.back()->low_conn[0],edges.back()->low_conn.size(),k1,hm);
			k2 = Mesh::getNext(&edges.back()->low_conn[0],edges.back()->low_conn.size(),k1,hm);
			if( edges.back()->low_conn[k1] != prev ) 
				prev = static_cast<Node*>(edges.back()->low_conn[k1]); 
			else 
				prev = static_cast<Node*>(edges.back()->low_conn[k2]);
			bool found = false;
			for(adj_iterator it = prev->high_conn.begin(); it != prev->high_conn.end(); ++it) if( !(*it)->GetMarker(hm) )
			{
				if( *it == first && q != 0)
				{
					found = done = true;
					break;
				}
				if( (*it)->GetMarker(edge_set) )
				{
					q++;
					found = true;
					edges.push_back(static_cast<Edge *>(*it));
					(*it)->RemMarker(edge_set);
					break;
				}
			}
			if( !found ) throw Failure;
		}
		m->ReleaseMarker(edge_set);
		
		

		
		for(unsigned j = 0; j < nunite; j++)
			unite[j]->SetMarker(rem);
		
		if( !unite[0]->Hide() ) //we can't hide elements
			for(dynarray<Cell *,64>::iterator it = high_conn_set.begin(); it != high_conn_set.end(); it++) //untie every face from cell
			{
				adj_type::iterator jt = (*it)->low_conn.begin();
				while( jt != (*it)->low_conn.end()) //don't need to check is it hidden
					if( (*jt)->GetMarker(rem) )
						jt = (*it)->low_conn.erase(jt);
					else jt++;
			}
		else unite[0]->Show();
		
		
		for(unsigned j = 0; j < nunite; j++) // delete all faces
		{
			if( !unite[j]->Hide() )
			{
				unite[j]->high_conn.clear(); //untie cells from face
				if( unite[j]->GetMarker(del_protect) )
					std::cout << __FUNCTION__ << " deleted protected face, united " << nunite << " faces " << std::endl;
				unite[j]->Delete(); 
			}
		}
		
		
		
		
		for(tiny_map<Edge *,int,64>::iterator it = edge_visit.begin(); it != edge_visit.end(); it++)
			if( it->second != 1 )
			{
				it->first->RemMarker(rem);
				if( !it->first->high_conn.empty() && Mesh::Count(&it->first->high_conn[0],it->first->high_conn.size(),hm) != 0 ) throw -1; //it's connected to face somewhere
				if( it->first->GetMarker(del_protect) )
					std::cout << __FUNCTION__ << " deleted protected edge, united " << nunite << " faces " << std::endl;
				it->first->Delete();
			}
			
		m->ReleaseMarker(rem);

		for(dynarray<Node *,64>::iterator it = nodes.begin(); it != nodes.end(); it++) //delete nodes inside the face
		{
			if( (*it)->high_conn.empty() || Mesh::Count(&(*it)->high_conn[0],(*it)->high_conn.size(),hm) == 0 )
			{
				if( !(*it)->Hide() )
				{
					adj_type::iterator jt = (*it)->low_conn.begin();
					while(jt != (*it)->low_conn.end() ) // iterate through cells of the node
					{
						adj_type::iterator qt = (*jt)->high_conn.begin(); //iterate through nodes of the cell
						while( qt != (*jt)->high_conn.end() )
						{
							if( *qt == *it ) 
								qt = (*jt)->high_conn.erase(qt); //remove links from the cell to the node
							else ++qt;
						}
						++jt;
					}
					(*it)->low_conn.clear(); // remove links to cells
					if( (*it)->GetMarker(del_protect) )
						std::cout << __FUNCTION__ << " deleted protected node, united " << nunite << " faces " << std::endl;
					(*it)->Delete();
				}
			}
		}
				
		Face * ret = m->CreateFace(&edges[0], edges.size()).first;
		
		
		
		if( ret->GetGeometricType() == MultiLine ) throw -1;
		
		for(dynarray<Cell *,64>::iterator it = high_conn_set.begin(); it != high_conn_set.end(); it++)  //tie new face to old cells
		{
			 ret->high_conn.push_back(*it); // connect new face to cells
			(*it)->low_conn.push_back(ret); // connect cells to new face
			
		}
		
		//~ if( m->HaveGeometricData(ORIENTATION,FACE) )
			//~ if( ret->FixNormalOrientation() ) std::cout << "fixed in unite " << std::endl;
		
		for(dynarray<Cell *,64>::iterator it = high_conn_set.begin(); it != high_conn_set.end(); it++)  //tie new face to old cells
		{
			(*it)->ComputeGeometricType();
			m->RecomputeGeometricData(*it);
		}
		
		return ret;
	}
	
	
	bool Face::TestUniteFaces(Face ** unite, INMOST_DATA_ENUM_TYPE nunite, MIDType del_protect)
	{
		if( nunite == 0 ) return false;
		bool doexit = false, dothrow = false;
		for(unsigned j = 0; j < nunite; j++)
			if( unite[j]->GetMarker(del_protect) ) doexit = true;
		Mesh * m = unite[0]->GetMeshLink();
		MIDType hm = m->HideMarker();
		
			
		if( doexit ) return false;
		
		dynarray<Cell *,64> high_conn_set;
		MIDType rem = m->CreateMarker();
		
		
		for(unsigned j = 0; j < nunite; j++)
			for(adj_type::iterator it = unite[j]->high_conn.begin(); it != unite[j]->high_conn.end(); it++) if( !(*it)->GetMarker(hm) )
				if( !(*it)->GetMarker(rem) )
				{
					high_conn_set.push_back(static_cast<Cell *>(*it));
					(*it)->SetMarker(rem);
				}
		
		for(unsigned j = 0; j < high_conn_set.size(); j++) high_conn_set[j]->RemMarker(rem);
		
		
		
		dynarray<Node *,64> nodes;
		tiny_map<Edge *, int,64> edge_visit;
		for(unsigned j = 0; j < nunite; j++)
		{
			for(adj_iterator it = unite[j]->low_conn.begin(); it != unite[j]->low_conn.end(); it++) if( !(*it)->GetMarker(hm) )
				edge_visit[(*it)->getAsEdge()]++;
		}
		for(tiny_map<Edge *,int,64>::iterator it = edge_visit.begin(); it != edge_visit.end(); it++)
		{
			if( it->second == 2 )
			{
				if( it->first->GetMarker(del_protect) ) doexit = true; // edge is protected
				it->first->SetMarker(rem);
				for(adj_iterator jt = it->first->low_conn.begin(); jt != it->first->low_conn.end(); jt++) if( !(*jt)->GetMarker(hm) )
					if( !(*jt)->GetMarker(rem) )
					{
						(*jt)->SetMarker(rem);
						nodes.push_back(static_cast<Node *>(*jt));
					}
			}
			else if( it->second != 1 )
			{
				doexit = true;
				dothrow = true;
			}
		}
		for(unsigned j = 0; j < nodes.size(); j++) 
		{
			nodes[j]->RemMarker(rem);
			if( nodes[j]->GetMarker(del_protect) )
			{
				int nonzero = 0;
				for(adj_iterator it = nodes[j]->high_conn.begin(); it != nodes[j]->high_conn.end(); it++) if( !(*it)->GetMarker(hm) )//iterate through edges of the node
					if( !(*it)->GetMarker(rem) ) nonzero++; // check if edge should not be deleted
				if( nonzero == 0 ) //all edges are deleted but the node is protected
					doexit = true;
			}
		}
		for(tiny_map<Edge *,int,64>::iterator it = edge_visit.begin(); it != edge_visit.end(); it++)
			if( it->second != 1 ) it->first->RemMarker(rem);
		m->ReleaseMarker(rem);
		if( doexit )
		{
			if( dothrow ) throw Impossible; //something went the way it shouldn't, possibly bad input
			return false;
		}
		return true;
	}

	Edge * Edge::UniteEdges(Edge ** edges, INMOST_DATA_ENUM_TYPE nedges, MIDType del_protect)
	{
		if( nedges == 0 ) return NULL;
		Mesh * m = edges[0]->GetMeshLink();
		bool doexit = false, dothrow = false;
		MIDType hm = m->HideMarker();
		MIDType rem = m->CreateMarker();
		dynarray<Cell *,64> cells;
		dynarray<Face *,64> high_conn_faces;
		tiny_map<Node *,int,64> nodes;
		dynarray<Node *,64> build_nodes;
		for(Edge ** it = edges; it != edges+nedges; ++it)
		{
			if( (*it)->GetMarker(del_protect) )
				doexit = true;
			for(adj_iterator jt = (*it)->high_conn.begin(); jt != (*it)->high_conn.end(); ++jt) if( !(*jt)->GetMarker(hm) )
			{
				if( !(*jt)->GetMarker(rem) )
				{
					high_conn_faces.push_back(static_cast<Face *>(*jt));
					(*jt)->SetMarker(rem);
				}
			}
			for(adj_iterator jt = (*it)->low_conn.begin(); jt != (*it)->low_conn.end(); ++jt) if( !(*jt)->GetMarker(hm) )
				nodes[(*jt)->getAsNode()]++;
		}
		
		for(dynarray<Face *,64>::iterator it = high_conn_faces.begin(); it != high_conn_faces.end(); ++it) 
		{
			(*it)->RemMarker(rem);
			for(adj_iterator jt = (*it)->high_conn.begin(); jt != (*it)->high_conn.end(); ++jt)
				if( !(*jt)->GetMarker(rem) )
				{
					(*jt)->SetMarker(rem);
					cells.push_back(static_cast<Cell *>(*jt));
				}
		}

		if( doexit )
		{
			m->ReleaseMarker(rem);
			return NULL;
		}
		
		for(tiny_map<Node *,int,64>::iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			if( it->second == 1 )
				build_nodes.push_back(it->first);
			else if( it->second == 2 )
			{
				if( it->first->GetMarker(del_protect) )
					doexit = true;
			}
			else 
			{
				doexit = true;
				dothrow = true;
			}
		}

		if( doexit )
		{
			m->ReleaseMarker(rem);
			if( dothrow ) throw Impossible; // bad input
			return NULL;
		}


		dynarray<unsigned,64> insert_pos; //position where we insert new edge

		for(Edge ** it = edges; it != edges+nedges; ++it) (*it)->SetMarker(rem);

		for(dynarray<Face *,64>::iterator it = high_conn_faces.begin(); it != high_conn_faces.end(); ++it)
			for(unsigned k = 0; k < (*it)->low_conn.size(); k++) //insert new edge to the first position where we delete old edge
				if( (*it)->low_conn[k]->GetMarker(rem) )
				{
					//all united edges should appear in consecutive order in deleted face
					unsigned sum = 0, j = k;
					while( !(*it)->low_conn[j]->GetMarker(hm) && (*it)->low_conn[j]->GetMarker(rem) )
					{
						sum++; j++;
					}
					if( sum != nedges )
					{
						doexit = true;
						dothrow = true;
					}
					insert_pos.push_back(k);
					break;
				}

		if( doexit )
		{
			for(Edge ** it = edges; it != edges+nedges; ++it) (*it)->RemMarker(rem);
			m->ReleaseMarker(rem);
			if( dothrow ) throw Impossible; // bad input
			return NULL;
		}

		if( !edges[0]->Hide() ) //disconnect if cannot hide
		{
			for(dynarray<Face *,64>::iterator it = high_conn_faces.begin(); it != high_conn_faces.end(); ++it)
			{
				adj_iterator jt = (*it)->low_conn.begin(); //iterate over edges of faces
				while( jt != (*it)->low_conn.end())
				{
					if( (*jt)->GetMarker(rem) )
						jt = (*it)->low_conn.erase(jt);
					else ++jt;
				}
			}
		}
		else edges[0]->Show();


		for(Edge ** it = edges; it != edges+nedges; ++it) //delete edges
		{
			(*it)->RemMarker(rem);
			if( !(*it)->Hide() ) //cannot hide
			{
				(*it)->high_conn.clear(); //remove connection from edge to faces
				(*it)->Delete();
			}
		}

		m->ReleaseMarker(rem);

		for(tiny_map<Node *,int,64>::iterator it = nodes.begin(); it != nodes.end(); it++)
			if( it->second != 1 )
			{
				if( !it->first->Hide() ) //cannot hide, we have to untie cells from nodes
				{
					adj_iterator qt = it->first->low_conn.begin();
					while(qt != it->first->low_conn.end()) //iterate through cells of the node
					{
						adj_iterator jt =(*qt)->high_conn.begin();
						while(jt != (*qt)->high_conn.end() ) // iterate through nodes of the cell
						{
							if( *jt == it->first )
								jt = (*qt)->high_conn.erase(jt); //erase link to node
							else ++jt;
						}
						++qt;
					}
					it->first->Delete();
				}
			}

			Edge * e = m->CreateEdge(&build_nodes[0], build_nodes.size()).first;
		
		for(unsigned k = 0; k < insert_pos.size(); k++)
		{
			high_conn_faces[k]->low_conn.insert(high_conn_faces[k]->low_conn.begin()+insert_pos[k],e);
			e->high_conn.push_back(high_conn_faces[k]);
			high_conn_faces[k]->ComputeGeometricType();
			m->RecomputeGeometricData(high_conn_faces[k]);
		}

		for(dynarray<Cell *,64>::iterator it = cells.begin(); it != cells.end(); ++it)
		{
			(*it)->ComputeGeometricType();
			m->RecomputeGeometricData(*it);
		}


		return e;
	}
	bool Edge::TestUniteEdges(Edge ** edges, INMOST_DATA_ENUM_TYPE nedges, MIDType del_protect)
	{
		if( nedges == 0 ) return false;
		Mesh * m = edges[0]->GetMeshLink();
		bool doexit = false, dothrow = false;
		MIDType hm = m->HideMarker();
		MIDType rem = m->CreateMarker();
		dynarray<Face *,64> high_conn_faces;
		tiny_map<Node *,int,64> nodes;
//		dynarray<Node *,64> build_nodes;
		for(Edge ** it = edges; it != edges+nedges; ++it)
		{
			if( (*it)->GetMarker(del_protect) )
				doexit = true;
			for(adj_iterator jt = (*it)->high_conn.begin(); jt != (*it)->high_conn.end(); ++jt) if( !(*jt)->GetMarker(hm) )
			{
				if( !(*jt)->GetMarker(rem) )
				{
					high_conn_faces.push_back(static_cast<Face *>(*jt));
					(*jt)->SetMarker(rem);
				}
			}
			for(adj_iterator jt = (*it)->low_conn.begin(); jt != (*it)->low_conn.end(); ++jt) if( !(*jt)->GetMarker(hm) )
				nodes[(*jt)->getAsNode()]++;
		}
		
		for(dynarray<Face *,64>::iterator it = high_conn_faces.begin(); it != high_conn_faces.end(); ++it) 
			(*it)->RemMarker(rem);

		if( doexit )
		{
			m->ReleaseMarker(rem);
			return false;
		}
		
		for(tiny_map<Node *,int,64>::iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			if( it->second == 1 )
			{
//				build_nodes.push_back(it->first);
			}
			else if( it->second == 2 )
			{
				if( it->first->GetMarker(del_protect) )
					doexit = true;
			}
			else 
			{
				doexit = true;
				dothrow = true;
			}
		}

		if( doexit )
		{
			m->ReleaseMarker(rem);
			if( dothrow ) throw Impossible; // bad input
			return false;
		}



		for(Edge ** it = edges; it != edges+nedges; ++it) (*it)->SetMarker(rem);

		for(dynarray<Face *,64>::iterator it = high_conn_faces.begin(); it != high_conn_faces.end(); ++it)
			for(unsigned k = 0; k < (*it)->low_conn.size(); k++) //insert new edge to the first position where we delete old edge
				if( (*it)->low_conn[k]->GetMarker(rem) )
				{
					//all united edges should appear in consecutive order in deleted face
					unsigned sum = 0, j = k;
					while( !(*it)->low_conn[j]->GetMarker(hm) && (*it)->low_conn[j]->GetMarker(rem) )
					{
						sum++; j++;
					}
					if( sum != nedges )
					{
						doexit = true;
						dothrow = true;
					}
					break;
				}

		for(Edge ** it = edges; it != edges+nedges; ++it) (*it)->RemMarker(rem);

		if( doexit )
		{
			m->ReleaseMarker(rem);
			if( dothrow ) throw Impossible; // bad input
			return false;
		}

		return true;
	}


	dynarray<Edge *,32> Edge::SplitEdge(Edge * e, Node ** nodes, INMOST_DATA_ENUM_TYPE nnodes, MIDType del_protect)
	{
		Mesh * m = e->GetMeshLink();
		dynarray<Edge *,32> ret;
		dynarray<Face *,64> faces;
		dynarray<Cell *,128> cells;
		Node * n[2];
		if( e->GetMarker(del_protect) || nnodes == 0 ) return ret;
		ret.reserve(nnodes+1);

		MIDType hm = m->HideMarker();
		MIDType dup = m->CreateMarker();

		for(adj_iterator it = e->high_conn.begin(); it != e->high_conn.end(); ++it) if( !(*it)->GetMarker(hm) )
		{
			faces.push_back(static_cast<Face *>(*it));
			for(adj_iterator jt = (*it)->high_conn.begin(); jt != (*it)->high_conn.end(); ++jt) if( !(*jt)->GetMarker(hm) )
				if( !(*jt)->GetMarker(dup) )
				{
					cells.push_back(static_cast<Cell *>(*jt));
					(*jt)->SetMarker(dup);
				}
		}

		for(dynarray<Cell *,128>::iterator it = cells.begin(); it != cells.end(); ++it)
			(*it)->RemMarker(dup);

		m->ReleaseMarker(dup);

		int k = 0;

		for(adj_iterator it = e->low_conn.begin(); it != e->low_conn.end(); ++it)
			if( !(*it)->GetMarker(hm) )
				n[k++] = static_cast<Node*>(*it);

		dynarray<unsigned,64> insert_pos; //position where we insert new edges

		for(dynarray<Face *,64>::iterator it = faces.begin(); it != faces.end(); ++it)
			for(adj_iterator jt = (*it)->low_conn.begin(); jt != (*it)->low_conn.end(); ++jt) if( !(*jt)->GetMarker(hm) )
				if( *jt == e )
				{
					insert_pos.push_back(jt-(*it)->low_conn.begin());
					break;
				}
		

		if( !e->Hide() ) //we cannot hide the edge, should disconnect faces
		{
			for(adj_iterator it = e->high_conn.begin(); it != e->high_conn.end(); ++it)
			{
				adj_iterator jt = (*it)->low_conn.begin();
				while(jt != (*it)->low_conn.end())
					if( *jt == e ) jt = (*it)->low_conn.erase(jt);
					else ++jt;
			}
			e->high_conn.clear();
			e->Delete();
		}
		
		{
			Node * build_nodes[2];
			build_nodes[0] = n[0];
			build_nodes[1] = nodes[0];
			ret.push_back(m->CreateEdge(build_nodes, 2).first);
			
			for(unsigned k = 0; k < nnodes-1; k++)
			{
				build_nodes[0] = nodes[k];
				build_nodes[1] = nodes[k+1];
				ret.push_back(m->CreateEdge(build_nodes, 2).first);
			}

			build_nodes[0] = nodes[nnodes-1];
			build_nodes[1] = n[1];
			ret.push_back(m->CreateEdge(build_nodes, 2).first);
		}

		//connect new edges to faces
		for(dynarray<Face *,64>::iterator it = faces.begin(); it != faces.end(); ++it)
		{
			(*it)->low_conn.insert((*it)->low_conn.begin()+insert_pos[it-faces.begin()],ret.begin(),ret.end());
			(*it)->ComputeGeometricType();
			m->RecomputeGeometricData(*it);
		}

		for(dynarray<Cell *,128>::iterator it = cells.begin(); it != cells.end(); ++it)
		{
			(*it)->ComputeGeometricType();
			m->RecomputeGeometricData(*it);
		}

		return ret;
	}
	bool Edge::TestSplitEdge(Edge * e, Node ** nodes, INMOST_DATA_ENUM_TYPE nnodes, MIDType del_protect)
	{
		return nnodes != 0 && !e->GetMarker(del_protect);
	}

	/*
	std::vector<Edge *> traverse_edges_sub(Edge * start, Edge * current, MIDType edgeset, MIDType visited)
	{
		//~ if( current == start ) return std::vector<Edge *> (1,start);
		std::vector< std::vector<Edge *> > paths;
		adjacent<Node> n = current->getNodes();
		for(unsigned j = 0; j < n.size(); j++)
		{
			if( !n[j].GetMarker(visited) )
			{
				n[j].SetMarker(visited);
				adjacent<Edge> e = n[j].getEdges();
				for(unsigned i = 0; i < e.size(); i++) 
				{
					if( &e[i] == start ) 
					{
						n[j].RemMarker(visited);
						return std::vector<Edge *> (1,start);
					}
					if( e[i].GetMarker(edgeset) && !e[i].GetMarker(visited) )
					{
						e[i].SetMarker(visited);
						std::vector<Edge *> ret = traverse_edges_sub(start,&e[i],edgeset,visited);
						e[i].RemMarker(visited);
						if( !ret.empty() )  
						{
							ret.push_back(&e[i]);
							paths.push_back(ret);
						}
					}
				}
				n[j].RemMarker(visited);
			}
		}
		if( !paths.empty() )
		{
			unsigned min = 0;
			for(unsigned j = 1; j < paths.size(); j++)
			{
				if( paths[j].size() < paths[min].size() )
					min = j;
			}
			return paths[min];
		}
		return std::vector<Edge *>();
	}
	//This function may be slow, because we collect all the arrays
	//should detect shortest path here, then collect one array with shortest path
	std::vector<Edge *> traverse_edges(Edge * start, MIDType edgeset, MIDType visited)
	{
		std::vector< std::vector<Edge *> > paths;
		adjacent<Node> n = start->getNodes();
		start->SetMarker(visited);
		//~ std::cout << "start edge " << start << std::endl;
		for(unsigned j = 0; j < n.size(); j++)
		{
			if( !n[j].GetMarker(visited) )
			{
				//~ std::cout  << "enter to the bridge " << &n[j] << " " << n[j].Coords()[0] << "," << n[j].Coords()[1] << "," << n[j].Coords()[2] << std::endl;
				n[j].SetMarker(visited);
				adjacent<Edge> e = n[j].getEdges();
				for(unsigned i = 0; i < e.size(); i++) 
					if( e[i].GetMarker(edgeset) && !e[i].GetMarker(visited) )
					{
						e[i].SetMarker(visited);
						std::vector<Edge *> ret = traverse_edges_sub(start,&e[i],edgeset,visited);
						e[i].RemMarker(visited);
						if( !ret.empty() )  
						{
							ret.push_back(&e[i]);
							paths.push_back(ret);
						}
					}
				n[j].RemMarker(visited);
			}
		}
		start->RemMarker(visited);
		if( !paths.empty() )
		{
			unsigned min = 0;
			for(unsigned j = 1; j < paths.size(); j++)
			{
				if( paths[j].size() < paths[min].size() )
					min = j;
			}
			return paths[min];
		}
		return std::vector<Edge *>();
	}
	*/
	
	template<class T>
	class incident_matrix //maybe update from projects/OctreeCutcell/octgrid.cpp
	{
		dynarray< unsigned char, 4096 > matrix;
		dynarray< char ,256 > visits;
		dynarray< T , 256> head_column;
		dynarray<Element *, 256> head_row;
		dynarray<unsigned char ,256> head_row_count;
		dynarray<unsigned, 256> insert_order;
		bool exit_recurse;
		dynarray<T,64> min_loop; //used as return
		dynarray< char , 256 > hide_column;
		dynarray< char , 256 > hide_row;
		dynarray< char , 256 > stub_row;
		bool do_hide_row(unsigned k)
		{
			if( hide_column[k] == 0 )
			{
				hide_column[k] = 1;
				for(unsigned i = 0; i < head_row_count.size(); i++)
				if( matrix[k*head_row_count.size()+i] == 1 )
				{
					head_row_count[i] -= 1;
					if( head_row_count[i] == 0 ) 
					{
						hide_row[i] = 1;
						stub_row[i] = 0;
					}
				}
				insert_order.pop_back();
			} 
			return true;
		}
		
		bool do_show_row(unsigned k)
		{
			if( hide_column[k] == 1 )
			{
				hide_column[k] = 0;
				
				bool success = true;
				for(unsigned i = 0; i < head_row_count.size(); i++)
				if( matrix[k*head_row_count.size()+i] == 1 )
				{
					head_row_count[i] += 1;
					if( head_row_count[i] > 0 ) hide_row[i] = 0;
					if( head_row_count[i] > 2 ) success = false;
				}
				insert_order.push_back(k);
				if( !success ) do_hide_row(k);
				return success;
				
			} else return true;
		}
		bool test_success()
		{
			bool success = true;
			for(unsigned j = 0; j < head_row_count.size(); j++)
			{
				if( head_row_count[j] == 1 )
				{
					success = false;
					break;
				}
			}
			return success;
		}
		void recursive_find(unsigned node, unsigned length)
		{
			if( !min_loop.empty() && length >= min_loop.size() ) return;
			bool success = false;
			if( do_show_row(node) )
			{
				success = test_success();
				
				if( success )
				{
					if( min_loop.empty() || min_loop.size() > length ) 
					{
						min_loop.clear();
						for(unsigned j = 0; j < insert_order.size(); j++)
							min_loop.push_back(static_cast<T>(head_column[insert_order[j]]));
						if( min_loop.size() == head_column.size() ) // all elements were visited
						{
							unsigned num = 0; 
							for(unsigned j = 0; j < head_row.size(); j++) //check that all bridge elements were visited - we don't have any other loop then
								num += hide_row[j];
							if( num == head_row.size() ) exit_recurse = true; //exit recursive loop
						}
					}
				}
				else
				{
					bool stub = false;
					for(unsigned j = 0; j < head_row_count.size() && !exit_recurse; j++) //first try follow the order
					{
						if( stub_row[j] == 0 && matrix[node*head_row_count.size()+j] == 1 && head_row_count[j] == 1 )
						{
							for(unsigned q = 0; q < head_column.size() && !exit_recurse; q++)
							{
								if( visits[q] > 0 && matrix[q*head_row_count.size()+j] == 1 && hide_column[q] == 1 ) 
								{
									recursive_find(q,length+1);
								}
							}
							if( head_row_count[j] == 1 )
							{
								stub_row[j] = 1;
								stub = true;
								break; //this is a stub path
							} 
						}
					}
				
					if( !stub ) for(unsigned j = 0; j < head_row_count.size() && !exit_recurse; j++)
					{
						if( stub_row[j] == 0 && matrix[node*head_row_count.size()+j] == 0 && head_row_count[j] == 1 )
						{
							for(unsigned q = 0; q < head_column.size() && !exit_recurse; q++)
							{
								if( visits[q] > 0 && matrix[q*head_row_count.size()+j] == 1 && hide_column[q] == 1 ) 
								{
									recursive_find(q,length+1);
								}
							}
							if( head_row_count[j] == 1 ) 
							{
								stub_row[j] = 1;
								stub = true;
								break; //this is a stub path
							}
						}
					}
					
				}
				do_hide_row(node);
			}
			if( length == 1 )
			{
				for(unsigned j = 0; j < head_row.size(); j++)
					stub_row[j] = 0;
			}
		}
	public:
		bool all_visited()
		{
			for(unsigned k = 0; k < visits.size(); k++)
				if( visits[k] != 0 ) return false;
			return true;
		}
		void print_matrix()
		{
			Storage::real cnt[3];
			for(unsigned k = 0; k < head_column.size(); k++)
			{
				for(unsigned j = 0; j < head_row.size(); j++)
					std::cout << static_cast<int>(matrix[k*head_row.size()+ j]);
				std::cout << " " << (int)visits[k];
				head_column[k]->Centroid(cnt);
				std::cout << " " << cnt[0] << " " << cnt[1] << " " << cnt[2];
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}
		template<typename InputIterator>
		incident_matrix(InputIterator beg, InputIterator end, unsigned num_inner)
		: head_column(beg,end), min_loop()
		{
			isInputForwardIterators<T,InputIterator>();
			Mesh * m = head_column[0]->GetMeshLink();
			MIDType hide_marker = m->CreateMarker();

			visits.resize(head_column.size());
			for(typename dynarray<T, 256>::iterator it = head_column.begin(); it != head_column.end(); ++it)
			{
				unsigned k = it-head_column.begin();
				visits[k] = k < num_inner ? 2 : 1;
				adjacent<Element> sub = (*it)->getAdjElements((*it)->GetElementType() >> 1);
				for(adjacent<Element>::iterator jt = sub.begin(); jt != sub.end(); ++jt)
					if( !jt->GetMarker(hide_marker) )
					{
						head_row.push_back(&*jt);
						jt->SetMarker(hide_marker);
					}
			}
			
			
			
			tiny_map<Element *,int,256> mat_num;
			
			for(dynarray<Element *,256>::iterator it = head_row.begin(); it != head_row.end(); ++it)
			{
				(*it)->RemMarker(hide_marker);
				mat_num[*it] = (int)(it-head_row.begin());
			}	
			
			m->ReleaseMarker(hide_marker);
				
			matrix.resize(head_row.size()*head_column.size(),0);
			
			
			for(typename dynarray<T,256>::iterator it = head_column.begin(); it != head_column.end(); ++it)
			{
				adjacent<Element> sub = (*it)->getAdjElements((*it)->GetElementType() >> 1);
				for(adjacent<Element>::iterator jt = sub.begin(); jt != sub.end(); ++jt)
				{
					matrix[(it-head_column.begin())*head_row.size()+mat_num[&*jt]] = 1;
				}
			}
			
			head_row_count.resize(head_row.size(),0);
			
			stub_row.resize(head_row.size(),0);
			hide_row.resize(head_row.size(),1);
			hide_column.resize(head_column.size(),1);
		}
		incident_matrix(const incident_matrix & other) 
		: matrix(other.matrix), head_column(other.head_column), head_row(other.head_row), 
		  head_row_count(other.head_row_count), min_loop(other.min_loop), 
		  hide_row(other.hide_row), hide_column(other.hide_column), 
		  stub_row(other.stub_row) 
		{
		}
		incident_matrix & operator =(const incident_matrix & other) 
		{
			matrix = other.matrix; 
			head_column = other.head_column; 
			head_row = other.head_row; 
			head_row_count = other.head_row_count; 
			min_loop = other.min_loop; 
			hide_row = other.hide_row; 
			hide_column = other.hide_column;
			stub_row = other.stub_row;
			return *this;
		}
		~incident_matrix()
		{
		}
		void find_shortest_loop(dynarray<T,64> & ret)
		{
			ret.clear();
			exit_recurse = false;
			unsigned first = UINT_MAX;
			do
			{
				first = UINT_MAX;
				for(unsigned q = 0; q < head_column.size(); q++)
					if( visits[q] == 1 )
					{
						first = q;
						break;
					}
				if( first != UINT_MAX )
				{
					recursive_find(first,1);
					if( min_loop.empty() )
						visits[first]--; //don't start again from this element
				}
			} while( min_loop.empty() && first != UINT_MAX );
			
			ret.insert(ret.end(),min_loop.begin(),min_loop.end());
			min_loop.clear();
			
			if( !ret.empty() )
			{
				Mesh * m = ret[0]->GetMeshLink();
				MIDType hide_marker = m->CreateMarker();
				for(unsigned k = 0; k < ret.size(); k++) ret[k]->SetMarker(hide_marker);
				for(unsigned k = 0; k < head_column.size(); k++)
					if( head_column[k]->GetMarker(hide_marker) ) visits[k]--;
				for(unsigned k = 0; k < ret.size(); k++) ret[k]->RemMarker(hide_marker);
				m->ReleaseMarker(hide_marker);
			}
		}
	};


	dynarray<Face *,32> Face::SplitFace(Face * face, Edge ** edges, INMOST_DATA_ENUM_TYPE nedges, MIDType del_protect)
	{
		dynarray<Edge *,64> loop;
		dynarray<Face *,32> ret;
		dynarray<Edge *,128> temp;
		if( nedges == 0 || face->GetMarker(del_protect) ) return ret;
		Mesh * m = face->GetMeshLink();
		MIDType hm = m->HideMarker();
		unsigned input_edges = nedges;
		dynarray<Cell *,2> cells;

		for(adj_iterator it = face->high_conn.begin(); it != face->high_conn.end(); ++it) if( !(*it)->GetMarker(hm) )
			cells.push_back(static_cast<Cell *>(*it));

		temp.insert(temp.end(),edges,edges+nedges);
		for(adj_iterator it = face->low_conn.begin(); it != face->low_conn.end(); ++it) if( !(*it)->GetMarker(hm) )
			temp.push_back(static_cast<Edge *>(*it));

		incident_matrix<Edge *> mat(temp.begin(),temp.end(),input_edges);
		
		if( !face->Hide() )
		{
			for(unsigned k = 0; k < cells.size(); k++)
				for(adj_iterator it = cells[k]->low_conn.begin(); it != cells[k]->low_conn.end(); ++it)
					if( *it == face )
					{
						cells[k]->low_conn.erase(it);
						break;
					}
			face->high_conn.clear();
			face->Delete();
		}
		
		do
		{
			mat.find_shortest_loop(loop);
			if (!loop.empty()) ret.push_back(m->CreateFace(&loop[0], loop.size()).first);
			else break;
		} while(true);
		

		


		for(dynarray<Cell *,2>::iterator it = cells.begin(); it != cells.end(); ++it)
		{
			(*it)->high_conn.clear(); //have to recompute cell nodes
			(*it)->low_conn.insert((*it)->low_conn.end(),ret.begin(),ret.end());
			(*it)->ComputeGeometricType();
			dynarray<Node *,64> nn;
			nn.reserve(128);
			m->RestoreCellNodes(*it,nn);
			(*it)->high_conn.insert((*it)->high_conn.begin(),nn.begin(),nn.end());
			m->RecomputeGeometricData(*it);
		}

		return ret;
	}
	bool Face::TestSplitFace(Face * face, Edge ** edges, INMOST_DATA_ENUM_TYPE nedges, MIDType del_protect)
	{
		return edges != 0 && !face->GetMarker(del_protect);
	}


	dynarray<Cell *,32> Cell::SplitCell(Cell * cell, Face ** faces, INMOST_DATA_ENUM_TYPE nfaces, MIDType del_protect)
	{
		dynarray<Cell *,32> ret;
		dynarray<Face *,64> loop;
		dynarray<Face *,128> temp;
		if( nfaces == 0 || cell->GetMarker(del_protect) ) return ret;
		Mesh * m = cell->GetMeshLink();
		MIDType hm = m->HideMarker();
		unsigned input_faces = nfaces;

		temp.insert(temp.end(),faces,faces+nfaces);
		for(adj_iterator it = cell->low_conn.begin(); it != cell->low_conn.end(); ++it) if( !(*it)->GetMarker(hm) )
			temp.push_back(static_cast<Face *>(*it));
			
		incident_matrix<Face *> mat(temp.begin(),temp.end(),input_faces);

		cell->Delete();
			
		do
		{
			mat.find_shortest_loop(loop);
			if (!loop.empty()) ret.push_back(m->CreateCell(&loop[0], loop.size()).first);
			else break;
		} while( true );

		return ret;
	}
	bool Cell::TestSplitCell(Cell * cell, Face ** faces, INMOST_DATA_ENUM_TYPE nfaces, MIDType del_protect)
	{
		return nfaces != 0 && !cell->GetMarker(del_protect);
	}
	
	void Mesh::BeginModification()
	{
		hide_element = CreateMarker();
		new_element = CreateMarker();
	}
	
	void Mesh::SwapModification()
	{
		MIDType temp = hide_element;
		hide_element = new_element;
		new_element = temp;

		for(ElementType etype = EDGE; etype <= CELL; etype = etype << 1)
			for(Mesh::iteratorElement it = BeginElement(etype); it != EndElement(); ++it)
				if( it->GetMarker(new_element) )
				{
					it->ComputeGeometricType();
					RecomputeGeometricData(&*it);
				}
	}
	
	void Mesh::ApplyModification()
	{
		for(Mesh::iteratorTag it = BeginTag(); it != EndTag(); ++it)
			if( it->GetDataType() == DATA_REFERENCE )
			{
				for(ElementType etype = NODE; etype <= CELL; etype = etype << 1)
					if( it->isDefined(etype) )
					{
						if( it->isSparse(etype) )
						{
							for(Mesh::iteratorElement jt = BeginElement(etype); jt != EndElement(); ++jt)
								if( jt->HaveData(*it) )
								{
									Storage::reference_array arr = jt->ReferenceArray(*it);
									for(Storage::reference_array::iterator qt = arr.begin(); qt != arr.end(); ++qt)
										if( (*qt)->GetMarker(hide_element) ) *qt = NULL;
								}
						}
						else
						{
							for(Mesh::iteratorElement jt = BeginElement(etype); jt != EndElement(); ++jt)
							{
								Storage::reference_array arr = jt->ReferenceArray(*it);
								for(Storage::reference_array::iterator qt = arr.begin(); qt != arr.end(); ++qt)
									if( (*qt)->GetMarker(hide_element) ) *qt = NULL;
							}
						}
					}
			}
		for(Mesh::iteratorSet it = BeginSet(); it != EndSet(); it++)
		{
			ElementSet erase;
			for(ElementSet::iterator jt = it->begin(); jt != it->end(); jt++)
				if( jt->GetMarker(hide_element) )
					erase.Insert(&*jt);
			if( !erase.empty() ) it->Difference(erase);
		}
	}

	void Mesh::ResolveModification()
	{
		throw NotImplemented;
	}
	
	void Mesh::EndModification()
	{
		//ApplyModification();
		MIDType temp = hide_element;
		hide_element = 0;
		for(ElementType etype = CELL; etype >= NODE; etype = etype >> 1)
			for(Mesh::iteratorElement it = BeginElement(etype); it != EndElement(); ++it)
				if( it->GetMarker(temp) ) delete &*it;
				else if( it->GetMarker(new_element) ) it->RemMarker(new_element);
		ReleaseMarker(temp);
		ReleaseMarker(new_element);
		new_element = 0;
		
		if( have_global_id ) AssignGlobalID(have_global_id);
	}
	
	bool Element::Hide() 
	{
		if(GetMeshLink()->HideMarker()) 
		{
			SetMarker(GetMeshLink()->HideMarker()); 
			return true;
		} 
		return false;
	}
	
	bool Element::Show() 
	{
		if(GetMeshLink()->HideMarker()) 
		{
			if( GetMarker( GetMeshLink()->HideMarker() ) )
			{
				RemMarker(GetMeshLink()->HideMarker()); 
				return true;
			}
		} 
		return false;
	}
	
	bool Element::Delete() 
	{
		if(Hide()) 
		{
			if( GetElementType() != CELL ) //mark all elements that rely on this that they should be deleted
			{
				adjacent<Element> up = getAdjElements(GetElementType() << 1);
				for(adjacent<Element>::iterator it = up.begin(); it != up.end(); ++it)
					it->Delete();
			}
			return false;
		}
		else 
		{
			delete this; 
			return true;
		}
	}
	
	bool Element::Hidden()
	{
		return GetMarker(GetMeshLink()->HideMarker());
	}
	
	bool Element::New()
	{
		return GetMarker(GetMeshLink()->NewMarker());
	}
	
	
	INMOST_DATA_ENUM_TYPE Mesh::getNext(Element ** arr, INMOST_DATA_ENUM_TYPE size, INMOST_DATA_ENUM_TYPE k, MIDType marker)
	{
		k++;
		while(k < size && arr[k]->GetMarker(marker)) k++;
		return k;
	}
	
	INMOST_DATA_ENUM_TYPE Mesh::Count(Element ** arr, INMOST_DATA_ENUM_TYPE size, MIDType marker)
	{
		unsigned ret = 0, k = 0;
		while(k < size) 
		{
			if( !arr[k]->GetMarker(marker) ) ret++;
			k++;
		}
		return ret;
	}
}

#endif
