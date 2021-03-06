#include "inmost.h"
#include <iomanip>
#if defined(USE_MESH)
namespace INMOST
{

	const char * Element::GeometricTypeName(GeometricType t)
	{
		switch(t)
		{
			case Unset:        return "Unset";
			case Vertex:       return "Vertex";
			case Line:         return "Line";
			case MultiLine:    return "MultiLine";
			case Tri:          return "Tri";
			case Quad:         return "Quad";
			case Polygon:      return "Polygon";
			case MultiPolygon: return "MultiPolygon";
			case Tet:          return "Tet";
			case Hex:          return "Hex";
			case Prism:        return "Prism";
			case Pyramid:      return "Pyramid";
			case Polyhedron:   return "Polyhedron";
			case Set:          return "Set";
		};
		return "Unset";
	}
	
	
	Storage::integer Element::GetGeometricDimension(GeometricType m_type)
	{
		switch(m_type)
		{
			case Vertex:		return 0;
			case Line:			return 1;
			case MultiLine:
			case Tri:
			case Quad: 			
			case Polygon:		return 2;
			case MultiPolygon:
			case Tet:
			case Hex:
			case Prism:
			case Pyramid:
			case Polyhedron:	return 3;
			default: return UINT_MAX;
		}
		return UINT_MAX;
	}
	
	const char * Element::StatusName(Status s)
	{
		switch(s)
		{
			case Owned:  return "Owned";
			case Shared: return "Shared";
			case Ghost:  return "Ghost";
		}
		return "Unknown";
	}
	
	
	
	
	
	Storage::enumerator Element::nbAdjElements(ElementType _etype)const 
	{
		if( GetHandleElementType(GetHandle()) == ESET ) return getAsSet()->nbAdjElements(_etype);
		assert( !(_etype & MESH) );
		assert( !(_etype & ESET) );
		Mesh * mesh = GetMeshLink();
		dynarray<HandleType,128> result;
		integer conn[4] = {0,0,0,0};
		integer myconn, i;
		enumerator ret = 0;
		
		for(ElementType e = NODE, i = 0; e <= CELL; i++, e = e << 1)
		{
			if( _etype & e ) conn[i] = 1;
			if( GetElementType() & e ) myconn = i;
		}
		if( !mesh->HideMarker() )
		{
			for(i = 0; i < 4; i++) if( conn[i] )
			{
				if( i == myconn )
				{
					ret += 1;
				}
				else if( i == (myconn + 1 + 4)%4 )
				{
					ret += static_cast<enumerator>(mesh->HighConn(GetHandle()).size());
				}
				else if( i == (myconn - 1 + 4)%4 )
				{
					ret += static_cast<enumerator>(mesh->LowConn(GetHandle()).size());
				}
				else if( i == (myconn - 2 + 4)%4 )
				{
					MarkerType mrk = mesh->CreateMarker();
					if( (GetElementType() & NODE) || (GetElementType() & EDGE) )
					{
						adj_type const & hc = mesh->HighConn(GetHandle());
						for(adj_type::size_type it = 0; it < hc.size(); it++)
						{
							adj_type const & ihc = mesh->HighConn(hc[it]);
							for(adj_type::size_type jt = 0; jt < ihc.size(); jt++)
								if( !mesh->GetMarker(ihc[jt],mrk) )
								{
									result.push_back(ihc[jt]);
									mesh->SetMarker(ihc[jt],mrk);
								}
						}
						ret += static_cast<enumerator>(result.size());
					}
					else if( GetElementType() & FACE )
					{
						ret += static_cast<enumerator>(mesh->LowConn(GetHandle()).size());
					}
					else
					{
						adj_type const & lc = mesh->LowConn(GetHandle());
						for(adj_type::size_type it = 0; it < lc.size(); it++)
						{
							adj_type const & ilc = mesh->LowConn(lc[it]);
							for(adj_type::size_type jt = 0; jt < ilc.size(); jt++)
								if( !mesh->GetMarker(ilc[jt],mrk) )
								{
									result.push_back(ilc[jt]);
									mesh->SetMarker(ilc[jt],mrk);
								}
						}		
						ret += static_cast<enumerator>(result.size());
					}
					for(dynarray<HandleType,128>::size_type it = 0; it < result.size(); it++)
						mesh->RemMarker(result[it],mrk);
					result.clear();
					mesh->ReleaseMarker(mrk);
				}
			}
		}
		else
		{
			MarkerType hm = mesh->HideMarker();
			for(i = 0; i < 4; i++) if( conn[i] )
			{
				if( i == myconn )
				{
					if( !GetMarker(hm) ) ret ++;
				}
				else if( i == (myconn + 1 + 4)%4 )
				{
					adj_type const & hc = mesh->HighConn(GetHandle());
					for(adj_type::size_type it = 0; it < hc.size(); ++it)
						if( !mesh->GetMarker(hc[it],hm) ) ret++;
				}
				else if( i == (myconn - 1 + 4)%4 )
				{
					adj_type const & lc = mesh->LowConn(GetHandle());
					for(adj_type::size_type it = 0; it < lc.size(); ++it)
						if( !mesh->GetMarker(lc[it],hm) ) ret++;
				}
				else if( i == (myconn - 2 + 4)%4 )
				{
					MarkerType mrk = mesh->CreateMarker();
					if( (GetElementType() & NODE) || (GetElementType() & EDGE) )
					{
						adj_type const & hc = mesh->HighConn(GetHandle());
						for(adj_type::size_type it = 0; it < hc.size(); it++) if( !mesh->GetMarker(hc[it],hm) )
						{
							adj_type const & ihc = mesh->HighConn(hc[it]);
							for(adj_type::size_type jt = 0; jt < ihc.size(); jt++) if( !mesh->GetMarker(ihc[jt],hm) )
							{
								if( !mesh->GetMarker(ihc[jt],mrk) )
								{
									result.push_back(ihc[jt]);
									mesh->SetMarker(ihc[jt],mrk);
								}
							}
						}
						ret += static_cast<enumerator>(result.size());
					}
					else
					{
						adj_type const & lc = mesh->LowConn(GetHandle());
						for(adj_type::size_type it = 0; it < lc.size(); it++) if( !mesh->GetMarker(lc[it],hm) )
						{
							adj_type const & ilc = mesh->LowConn(lc[it]);
							for(adj_type::size_type jt = 0; jt < ilc.size(); jt++) if( !mesh->GetMarker(ilc[jt],hm) )
								if( !mesh->GetMarker(ilc[jt],mrk) )
								{
									result.push_back(ilc[jt]);
									mesh->SetMarker(ilc[jt],mrk);
								}
						}
						ret += static_cast<enumerator>(result.size());
					}
					for(dynarray<HandleType,128>::size_type it = 0; it < result.size(); it++)
						mesh->RemMarker(result[it],mrk);
					result.clear();
					mesh->ReleaseMarker(mrk);
				}
			}
		}
		return ret;
	}


	Storage::enumerator Element::nbAdjElements(ElementType _etype, MarkerType mask, bool invert)const 
	{
		if( GetHandleElementType(GetHandle()) == ESET ) return getAsSet()->nbAdjElements(_etype,mask,invert);
		assert( !(_etype & MESH) );
		assert( !(_etype & ESET) );
		Mesh * mesh = GetMeshLink();
		dynarray<HandleType,128> result;
		integer conn[4] = {0,0,0,0};
		integer myconn, i;
		enumerator ret = 0;
		
		for(ElementType e = NODE, i = 0; e <= CELL; i++, e = e << 1)
		{
			if( _etype & e ) conn[i] = 1;
			if( GetElementType() & e ) myconn = i;
		}
		if( !mesh->HideMarker() )
		{
			for(i = 0; i < 4; i++) if( conn[i] )
			{
				if( i == myconn )
				{
					if( invert ^ GetMarker(mask) ) ret += 1;
				}
				else if( i == (myconn + 1 + 4)%4 )
				{
					adj_type const & hc = mesh->HighConn(GetHandle());
					for(adj_type::size_type it = 0; it < hc.size(); ++it) 
						if( invert ^ mesh->GetMarker(hc[it],mask) ) ret++;
				}
				else if( i == (myconn - 1 + 4)%4 )
				{
					adj_type const & lc = mesh->LowConn(GetHandle());
					for(adj_type::size_type it = 0; it < lc.size(); ++it) 
						if( invert ^ mesh->GetMarker(lc[it],mask) ) ret++;
				}
				else if( i == (myconn - 2 + 4)%4 )
				{
					MarkerType mrk = mesh->CreateMarker();
					if( (GetElementType() & NODE) || (GetElementType() & EDGE) )
					{
						adj_type const & hc = mesh->HighConn(GetHandle());
						for(adj_type::size_type it = 0; it < hc.size(); it++)
						{
							adj_type const & ihc = mesh->HighConn(hc[it]);
							for(adj_type::size_type jt = 0; jt < ihc.size(); jt++)
								if( (invert ^ mesh->GetMarker(ihc[jt],mask)) && !mesh->GetMarker(ihc[jt],mrk) )
								{
									result.push_back(ihc[jt]);
									mesh->SetMarker(ihc[jt],mrk);
								}
						}
						ret += static_cast<enumerator>(result.size());
					}
					else
					{
						adj_type const & lc = mesh->LowConn(GetHandle());
						for(adj_type::size_type it = 0; it < lc.size(); it++)
						{
							adj_type const & ilc = mesh->LowConn(lc[it]);
							for(adj_type::size_type jt = 0; jt < ilc.size(); jt++)
								if( (invert ^ mesh->GetMarker(ilc[jt],mask)) && !mesh->GetMarker(ilc[jt],mrk) )
								{
									result.push_back(ilc[jt]);
									mesh->SetMarker(ilc[jt],mrk);
								}
						}
						ret += static_cast<enumerator>(result.size());
					}
					for(dynarray<HandleType,128>::size_type it = 0; it < result.size(); it++)
						mesh->RemMarker(result[it],mrk);
					result.clear();
					mesh->ReleaseMarker(mrk);
				}
			}
		}
		else
		{
			MarkerType hm = mesh->HideMarker();
			for(i = 0; i < 4; i++) if( conn[i] )
			{
				if( i == myconn )
				{
					if( !GetMarker(hm) && (invert ^ GetMarker(mask)) ) ret ++;
				}
				else if( i == (myconn + 1 + 4)%4 )
				{
					adj_type const & hc = mesh->HighConn(GetHandle());
					for(adj_type::size_type it = 0; it < hc.size(); ++it)
						if( !mesh->GetMarker(hc[it],hm) && (invert ^ mesh->GetMarker(hc[it],mask)) ) ret++;
				}
				else if( i == (myconn - 1 + 4)%4 )
				{
					adj_type const & lc = mesh->LowConn(GetHandle());
					for(adj_type::size_type it = 0; it < lc.size(); ++it)
						if( !mesh->GetMarker(lc[it],hm) && (invert ^ mesh->GetMarker(lc[it],mask)) ) ret++;
				}
				else if( i == (myconn - 2 + 4)%4 )
				{
					MarkerType mrk = mesh->CreateMarker();
					if( (GetElementType() & NODE) || (GetElementType() & EDGE) )
					{
						adj_type const & hc = mesh->HighConn(GetHandle());
						for(adj_type::size_type it = 0; it < hc.size(); it++) if( !mesh->GetMarker(hc[it],hm) )
						{
							adj_type const & ihc = mesh->HighConn(hc[it]);
							for(adj_type::size_type jt = 0; jt < ihc.size(); jt++) if( !mesh->GetMarker(ihc[jt],hm) )
								if( (invert ^ mesh->GetMarker(ihc[jt],mask)) && !mesh->GetMarker(ihc[jt],mrk) )
								{
									result.push_back(ihc[jt]);
									mesh->SetMarker(ihc[jt],mrk);
								}
						}
						ret += static_cast<enumerator>(result.size());
					}
					else
					{
						adj_type const & lc = mesh->LowConn(GetHandle());
						for(adj_type::size_type it = 0; it < lc.size(); it++) if( !mesh->GetMarker(lc[it],hm) )
						{
							adj_type const & ilc = mesh->LowConn(lc[it]);
							for(adj_type::size_type jt = 0; jt < ilc.size(); jt++) if( !mesh->GetMarker(ilc[jt],hm) )
								if( (invert ^ mesh->GetMarker(ilc[jt],mask)) && !mesh->GetMarker(ilc[jt],mrk) )
								{
									result.push_back(ilc[jt]);
									mesh->SetMarker(ilc[jt],mrk);
								}
						}		
						ret += static_cast<enumerator>(result.size());
					}
					for(dynarray<HandleType,128>::size_type it = 0; it < result.size(); it++)
						mesh->RemMarker(result[it],mrk);
					result.clear();
					mesh->ReleaseMarker(mrk);
				}
			}
		}
		return ret;
	}
	
	ElementArray<Element> Element::getAdjElements(ElementType _etype) const 
	{
		if( GetHandleElementType(GetHandle()) == ESET ) return getAsSet()->getAdjElements(_etype);
		assert( !(_etype & MESH) );
		assert( !(_etype & ESET) );
		INMOST::Mesh * mesh = GetMeshLink();
		ElementArray<Element> result(mesh);
		unsigned int conn[4] = {0,0,0,0};
		unsigned int myconn, i = 0;
		
		for(ElementType e = NODE; e <= CELL; e = e << 1)
		{
			if( _etype & e ) conn[i] = 1;
			if( GetElementType() & e ) myconn = i;
			i++;
		}
		if( !mesh->HideMarker() )
		{
			for(i = 0; i < 4; i++) if( conn[i] )
			{
				if( i == myconn )
				{
					result.push_back(GetHandle());
				}
				else if( i == (myconn + 1 + 4)%4 )
				{
					adj_type const & hc = mesh->HighConn(GetHandle());
					result.insert(result.end(),hc.begin(),hc.end());
				}
				else if( i == (myconn - 1 + 4)%4 )
				{
					adj_type const & lc = mesh->LowConn(GetHandle());
					result.insert(result.end(),lc.begin(),lc.end());
				}
				else if( i == (myconn - 2 + 4)%4 )
				{
					MarkerType mrk = mesh->CreateMarker();
					if( (GetElementType() & NODE) || (GetElementType() & EDGE) )
					{
						adj_type const & hc = mesh->HighConn(GetHandle());
						for(adj_type::size_type it = 0; it < hc.size(); it++)
						{
							adj_type const & ihc = mesh->HighConn(hc[it]);
							for(adj_type::size_type jt = 0; jt < ihc.size(); jt++)
								if( !mesh->GetMarker(ihc[jt],mrk) )
								{
									result.push_back(ihc[jt]);
									mesh->SetMarker(ihc[jt],mrk);
								}
						}
					}
					else
					{
						adj_type const & lc = mesh->LowConn(GetHandle());
						for(adj_type::size_type it = 0; it < lc.size(); it++)
						{
							adj_type const & ilc = mesh->LowConn(lc[it]);
							for(adj_type::size_type jt = 0; jt < ilc.size(); jt++)
								if( !mesh->GetMarker(ilc[jt],mrk) )
								{
									result.push_back(ilc[jt]);
									mesh->SetMarker(ilc[jt],mrk);
								}
						}
					}
					for(ElementArray<Element>::size_type it = 0; it < result.size(); it++)
						mesh->RemMarker(result.at(it),mrk);
					mesh->ReleaseMarker(mrk);
				}
			}
		}
		else
		{
			MarkerType hm = mesh->HideMarker();
			for(i = 0; i < 4; i++) if( conn[i] )
			{
				if( i == myconn )
				{
					if( !GetMarker(hm) )
						result.push_back(GetHandle());
				}
				else if( i == (myconn + 1 + 4)%4 )
				{
					adj_type const & hc = mesh->HighConn(GetHandle());
					for(adj_type::size_type it = 0; it < hc.size(); ++it)
						if( !mesh->GetMarker(hc[it],hm) ) result.push_back(hc[it]);
					
				}
				else if( i == (myconn - 1 + 4)%4 )
				{
					adj_type const & lc = mesh->LowConn(GetHandle());
					for(adj_type::size_type it = 0; it < lc.size(); ++it)
						if( !mesh->GetMarker(lc[it],hm) ) result.push_back(lc[it]);
				}
				else if( i == (myconn - 2 + 4)%4 )
				{
					MarkerType mrk = mesh->CreateMarker();
					if( (GetElementType() & NODE) || (GetElementType() & EDGE) )
					{
						adj_type const & hc = mesh->HighConn(GetHandle());
						for(adj_type::size_type it = 0; it < hc.size(); it++) if( !mesh->GetMarker(hc[it],hm) )
						{
							adj_type const & ihc = mesh->HighConn(hc[it]);
							for(adj_type::size_type jt = 0; jt < ihc.size(); jt++) if( !mesh->GetMarker(ihc[jt],hm) )
								if( !mesh->GetMarker(ihc[jt],mrk) )
								{
									result.push_back(ihc[jt]);
									mesh->SetMarker(ihc[jt],mrk);
								}
						}
					}
					else
					{
						adj_type const & lc = mesh->LowConn(GetHandle());
						for(adj_type::size_type it = 0; it < lc.size(); it++) if( !mesh->GetMarker(lc[it],hm) )
						{
							adj_type const & ilc = mesh->LowConn(lc[it]);
							for(adj_type::size_type jt = 0; jt != ilc.size(); jt++) if( !mesh->GetMarker(ilc[jt],hm) )
								if( !mesh->GetMarker(ilc[jt],mrk) )
								{
									result.push_back(ilc[jt]);
									mesh->SetMarker(ilc[jt],mrk);
								}
						}
					}
					for(ElementArray<Element>::size_type it = 0; it < result.size(); it++)
						mesh->RemMarker(result.at(it),mrk);
					mesh->ReleaseMarker(mrk);
				}
			}
		}
		return result;
	}
	
	ElementArray<Element> Element::getAdjElements(ElementType _etype, MarkerType mask, bool invert) const 
	{
		if( GetHandleElementType(GetHandle()) == ESET ) return getAsSet()->getAdjElements(_etype,mask,invert);
		assert( !(_etype & MESH) );
		assert( !(_etype & ESET) );
		INMOST::Mesh * mesh = GetMeshLink();
		ElementArray<Element> result(mesh);
		unsigned int conn[4] = {0,0,0,0};
		unsigned int myconn, i = 0;
		
		for(ElementType e = NODE; e <= CELL; e = e << 1)
		{
			if( _etype & e ) conn[i] = 1;
			if( GetElementType() & e ) myconn = i;
			i++;
		}
		if( !mesh->HideMarker() )
		{
			for(i = 0; i < 4; i++) if( conn[i] )
			{
				if( i == myconn )
				{
					if( invert ^ GetMarker(mask) ) result.push_back(GetHandle());
				}
				else if( i == (myconn + 1 + 4)%4 )
				{
					adj_type const & hc = mesh->HighConn(GetHandle());
					for(adj_type::size_type it = 0; it < hc.size(); ++it)
						if( invert ^ mesh->GetMarker(hc[it],mask) ) result.push_back(hc[it]);
					
				}
				else if( i == (myconn - 1 + 4)%4 )
				{
					adj_type const & lc = mesh->LowConn(GetHandle());
					for(adj_type::size_type it = 0; it < lc.size(); ++it)
						if( invert ^ mesh->GetMarker(lc[it],mask) ) result.push_back(lc[it]);
				}
				else if( i == (myconn - 2 + 4)%4 )
				{
					MarkerType mrk = mesh->CreateMarker();
					if( (GetElementType() & NODE) || (GetElementType() & EDGE) )
					{
						adj_type const & hc = mesh->HighConn(GetHandle());
						for(adj_type::size_type it = 0; it < hc.size(); it++)
						{
							adj_type const & ihc = mesh->HighConn(hc[it]);
							for(adj_type::size_type jt = 0; jt < ihc.size(); jt++)
								if( (invert ^ mesh->GetMarker(ihc[jt],mask)) && !mesh->GetMarker(ihc[jt],mrk) )
								{
									result.push_back(ihc[jt]);
									mesh->SetMarker(ihc[jt],mrk);
								}
						}
					}
					else
					{
						adj_type const & lc = mesh->LowConn(GetHandle());
						for(adj_type::size_type it = 0; it < lc.size(); it++)
						{
							adj_type const & ilc = mesh->LowConn(lc[it]);
							for(adj_type::size_type jt = 0; jt < ilc.size(); jt++)
								if( (invert ^ mesh->GetMarker(ilc[jt],mask)) && !mesh->GetMarker(ilc[jt],mrk) )
								{
									result.push_back(ilc[jt]);
									mesh->SetMarker(ilc[jt],mrk);
								}
						}
					}
					for(ElementArray<Element>::size_type it = 0; it < result.size(); it++)
						mesh->RemMarker(result.at(it),mrk);
					mesh->ReleaseMarker(mrk);
				}
			}
		}
		else
		{
			MarkerType hm = mesh->HideMarker();
			for(i = 0; i < 4; i++) if( conn[i] )
			{
				if( i == myconn )
				{
					if( !GetMarker(hm) && (invert ^ GetMarker(mask)) )
						result.push_back(GetHandle());
				}
				else if( i == (myconn + 1 + 4)%4 )
				{
					adj_type const & hc = mesh->HighConn(GetHandle());
					for(adj_type::size_type it = 0; it < hc.size(); ++it)
						if( (invert ^ mesh->GetMarker(hc[it],mask)) && !mesh->GetMarker(hc[it],hm) ) result.push_back(hc[it]);
					
				}
				else if( i == (myconn - 1 + 4)%4 )
				{
					adj_type const & lc = mesh->LowConn(GetHandle());
					for(adj_type::size_type it = 0; it < lc.size(); ++it)
						if( (invert ^ mesh->GetMarker(lc[it],mask)) && !mesh->GetMarker(lc[it],hm) ) result.push_back(lc[it]);
				}
				else if( i == (myconn - 2 + 4)%4 )
				{
					MarkerType mrk = mesh->CreateMarker();
					if( (GetElementType() & NODE) || (GetElementType() & EDGE) )
					{
						adj_type const & hc = mesh->HighConn(GetHandle());
						for(adj_type::size_type it = 0; it < hc.size(); it++) if( !mesh->GetMarker(hc[it],hm) )
						{
							adj_type const & ihc = mesh->HighConn(hc[it]);
							for(adj_type::size_type jt = 0; jt < ihc.size(); jt++) if( !mesh->GetMarker(ihc[jt],hm) )
								if( (invert ^ mesh->GetMarker(ihc[jt],mask)) && !mesh->GetMarker(ihc[jt],mrk) )
								{
									result.push_back(ihc[jt]);
									mesh->SetMarker(ihc[jt],mrk);
								}
						}
					}
					else
					{
						adj_type const & lc = mesh->LowConn(GetHandle());
						for(adj_type::size_type it = 0; it < lc.size(); it++) if( !mesh->GetMarker(lc[it],hm) )
						{
							adj_type const & ilc = mesh->LowConn(lc[it]);
							for(adj_type::size_type jt = 0; jt < ilc.size(); jt++) if( !mesh->GetMarker(ilc[jt],hm) )
								if( (invert ^ mesh->GetMarker(ilc[jt],mask)) && !mesh->GetMarker(ilc[jt],mrk) )
								{
									result.push_back(ilc[jt]);
									mesh->SetMarker(ilc[jt],mrk);
								}
						}
					}
					for(ElementArray<Element>::size_type it = 0; it < result.size(); it++)
						mesh->RemMarker(result.at(it),mrk);
					mesh->ReleaseMarker(mrk);
				}
			}
		}
		return result;
	}
	
	ElementArray<Node> Element::getNodes() const
	{
		switch(GetElementType())
		{
		case NODE: return ElementArray<Node>(GetMeshLink(),1,GetHandle());
		case EDGE: return getAsEdge()->getNodes();
		case FACE: return getAsFace()->getNodes();
		case CELL: return getAsCell()->getNodes();
		case ESET: return getAsSet()->getNodes();
		}
		return ElementArray<Node>(NULL);
	}
	ElementArray<Edge> Element::getEdges() const
	{
		switch(GetElementType())
		{
		case NODE: return getAsNode()->getEdges();
		case EDGE: return ElementArray<Edge>(GetMeshLink(),1,GetHandle());
		case FACE: return getAsFace()->getEdges();
		case CELL: return getAsCell()->getEdges();
		case ESET: return getAsSet()->getEdges();
		}
		return ElementArray<Edge>(NULL);
	}
	ElementArray<Face> Element::getFaces() const
	{
		switch(GetElementType())
		{
		case NODE: return getAsNode()->getFaces();
		case EDGE: return getAsEdge()->getFaces();
		case FACE: return ElementArray<Face>(GetMeshLink(),1,GetHandle());
		case CELL: return getAsCell()->getFaces();
		case ESET: return getAsSet()->getFaces();
		}
		return ElementArray<Face>(NULL);
	}
	ElementArray<Cell> Element::getCells() const
	{
		switch(GetElementType())
		{
		case NODE: return getAsNode()->getCells();
		case EDGE: return getAsEdge()->getCells();
		case FACE: return getAsFace()->getCells();
		case CELL: return ElementArray<Cell>(GetMeshLink(),1,GetHandle());
		case ESET: return getAsSet()->getCells();
		}
		return ElementArray<Cell>(NULL);
	}
	
	ElementArray<Node> Element::getNodes(MarkerType mask,bool invert) const
	{
		switch(GetElementType())
		{
		case NODE: return ElementArray<Node>(GetMeshLink(),invert ^ GetMarker(mask) ? 1 : 0,GetHandle());
		case EDGE: return getAsEdge()->getNodes(mask,invert);
		case FACE: return getAsFace()->getNodes(mask,invert);
		case CELL: return getAsCell()->getNodes(mask,invert);
		case ESET: return getAsSet()->getNodes(mask,invert);
		}
		return ElementArray<Node>(NULL);
	}
	ElementArray<Edge> Element::getEdges(MarkerType mask,bool invert) const
	{
		switch(GetElementType())
		{
		case NODE: return getAsNode()->getEdges(mask,invert);
		case EDGE: return ElementArray<Edge>(GetMeshLink(),invert ^ GetMarker(mask) ? 1 : 0,GetHandle());
		case FACE: return getAsFace()->getEdges(mask,invert);
		case CELL: return getAsCell()->getEdges(mask,invert);
		case ESET: return getAsSet()->getEdges(mask,invert);
		}
		return ElementArray<Edge>(NULL);	
	}
	ElementArray<Face> Element::getFaces(MarkerType mask,bool invert) const
	{
		switch(GetElementType())
		{
		case NODE: return getAsNode()->getFaces(mask,invert);
		case EDGE: return getAsEdge()->getFaces(mask,invert);
		case FACE: return ElementArray<Face>(GetMeshLink(),invert ^ GetMarker(mask) ? 1 : 0,GetHandle());
		case CELL: return getAsCell()->getFaces(mask,invert);
		case ESET: return getAsSet()->getFaces(mask,invert);
		}
		return ElementArray<Face>(NULL);
	}
	ElementArray<Cell> Element::getCells(MarkerType mask,bool invert) const
	{
		switch(GetElementType())
		{
		case NODE: return getAsNode()->getCells(mask,invert);
		case EDGE: return getAsEdge()->getCells(mask,invert);
		case FACE: return getAsFace()->getCells(mask,invert);
		case CELL: return ElementArray<Cell>(GetMeshLink(),invert ^ GetMarker(mask) ? 1 : 0,GetHandle());
		case ESET: return getAsSet()->getCells(mask,invert);
		}
		return ElementArray<Cell>(NULL);
	}
	
	bool Element::CheckElementConnectivity() const
	{
		Mesh * mesh = GetMeshLink();
		HandleType me = GetHandle();
		adj_type const & hc = mesh->HighConn(GetHandle());
		for(adj_type::size_type jt = 0; jt < hc.size(); jt++) //iterate over upper adjacent
		{
			bool found = false;
			adj_type const & ilc = mesh->LowConn(hc[jt]);
			for(adj_type::size_type kt = 0; kt < ilc.size(); kt++) //search for the link to me
				if( ilc[kt] == me )
				{
					found = true;
					break;
				}
			if( !found ) return false;
		}
		adj_type const & lc = mesh->LowConn(GetHandle());
		for(adj_type::size_type jt = 0; jt < lc.size(); jt++) //iterate over lower adjacent
		{
			bool found = false;
			adj_type const & ihc = mesh->HighConn(lc[jt]);
			for(adj_type::size_type kt = 0; kt < ihc.size(); kt++) //search for the link to me
				if( ihc[kt] == me )
				{
					found = true;
					break;
				}
			if( !found ) return false;
		}
		return true;
	}

	void Element::PrintElementConnectivity() const
	{
		Mesh * mesh = GetMeshLink();
		HandleType me = GetHandle();
		std::cout << "Element " << ElementTypeName(GetElementType()) << " " << LocalID() << std::endl;

		adj_type const & hc = mesh->HighConn(GetHandle());
		std::cout << "Upper adjacencies (" << hc.size() << "):" << std::endl;
		for(adj_type::size_type jt = 0; jt < hc.size(); jt++) //iterate over upper adjacent
		{
			std::cout << "[" << std::setw(3) << jt << "] " << ElementTypeName(GetHandleElementType(hc[jt])) << " " << GetHandleID(hc[jt]) << " lower ";
			bool found = false;
			adj_type const & ilc = mesh->LowConn(hc[jt]);
			std::cout << "(" << ilc.size() << "): ";
			for(adj_type::size_type kt = 0; kt < ilc.size(); kt++) //search for the link to me
			{
				std::cout << ElementTypeName(GetHandleElementType(ilc[kt])) << " " << GetHandleID(ilc[kt]) << " ";
				if( ilc[kt] == me ) found = true;
			}
			if( !found ) std::cout << " no me here! ";
			std::cout << std::endl;
		}
		adj_type const & lc = mesh->LowConn(GetHandle());
		std::cout << "Lower adjacencies (" << lc.size() << "):" << std::endl;
		for(adj_type::size_type jt = 0; jt < lc.size(); jt++) //iterate over lower adjacent
		{
			std::cout << "[" <<  std::setw(3) <<  jt << "] " << ElementTypeName(GetHandleElementType(lc[jt])) << " " << GetHandleID(lc[jt]) << " higher ";
			bool found = false;
			adj_type const & ihc = mesh->HighConn(lc[jt]);
			std::cout << "(" << ihc.size() << "): ";
			for(adj_type::size_type kt = 0; kt < ihc.size(); kt++) //search for the link to me
			{
				std::cout << ElementTypeName(GetHandleElementType(ihc[kt])) << " " << GetHandleID(ihc[kt]) << " ";
				if( ihc[kt] == me ) found = true;
			}
			if( !found ) std::cout << " no me here! ";
			std::cout << std::endl;
		}
	}
	
	bool Element::CheckConnectivity(Mesh * m)
	{
		for(Mesh::iteratorElement it = m->BeginElement(CELL | FACE | EDGE | NODE); it != m->EndElement(); it++)
			if( !it->CheckElementConnectivity() ) return false;
		return true;
	}
	
	
	
	ElementArray<Element> Element::BridgeAdjacencies(ElementType Bridge, ElementType Dest, MarkerType mask, bool invert) const
	{
		Mesh * m = GetMeshLink();
		MarkerType mrk = m->CreateMarker();
		ElementArray<Element> adjcells(m);
		ElementArray<Element> adjfaces = getAdjElements(Bridge);
		ElementArray<Element> my = Bridge & GetElementType() ? ElementArray<Element>(m) : getAdjElements(Dest);
		for(ElementArray<Element>::iterator it = my.begin(); it != my.end(); it++) it->SetMarker(mrk);
		for(ElementArray<Element>::iterator it = adjfaces.begin(); it != adjfaces.end(); it++)
		{
			ElementArray<Element> sub = it->getAdjElements(Dest);
			for(ElementArray<Element>::iterator jt = sub.begin(); jt != sub.end(); jt++)
				if( !jt->GetMarker(mrk) && (mask == 0 || (invert ^ jt->GetMarker(mask))) )
				{
					adjcells.push_back(*jt);
					jt->SetMarker(mrk);
				}
		}
		for(ElementArray<Element>::iterator it = adjcells.begin(); it != adjcells.end(); it++) it->RemMarker(mrk);
		for(ElementArray<Element>::iterator it = my.begin(); it != my.end(); it++) it->RemMarker(mrk);
		m->ReleaseMarker(mrk);
		return adjcells;
	}


	ElementArray<Node> Element::BridgeAdjacencies2Node(ElementType Bridge, MarkerType mask, bool invert) const
	{
		Mesh * m = GetMeshLink();
		MarkerType mrk = m->CreateMarker();
		ElementArray<Node> adjcells(m);
		ElementArray<Element> adjfaces = getAdjElements(Bridge);
		ElementArray<Element> my = Bridge & GetElementType() ? ElementArray<Element>(m) : getAdjElements(NODE);
		for(ElementArray<Element>::iterator it = my.begin(); it != my.end(); it++) it->SetMarker(mrk);
		for(ElementArray<Element>::iterator it = adjfaces.begin(); it != adjfaces.end(); it++)
		{
			ElementArray<Node> sub = it->getNodes();
			for(ElementArray<Node>::iterator jt = sub.begin(); jt != sub.end(); jt++)
				if( !jt->GetMarker(mrk) && (mask == 0 || (invert ^ jt->GetMarker(mask))) )
				{
					adjcells.push_back(*jt);
					jt->SetMarker(mrk);
				}
		}
		for(ElementArray<Node>::iterator it = adjcells.begin(); it != adjcells.end(); it++) it->RemMarker(mrk);
		for(ElementArray<Element>::iterator it = my.begin(); it != my.end(); it++) it->RemMarker(mrk);
		m->ReleaseMarker(mrk);
		return adjcells;
	}

	ElementArray<Edge> Element::BridgeAdjacencies2Edge(ElementType Bridge, MarkerType mask, bool invert) const
	{
		Mesh * m = GetMeshLink();
		MarkerType mrk = m->CreateMarker();
		ElementArray<Edge> adjcells(m);
		ElementArray<Element> adjfaces = getAdjElements(Bridge);
		ElementArray<Element> my = Bridge & GetElementType() ? ElementArray<Element>(m) : getAdjElements(EDGE);
		for(ElementArray<Element>::iterator it = my.begin(); it != my.end(); it++) it->SetMarker(mrk);
		for(ElementArray<Element>::iterator it = adjfaces.begin(); it != adjfaces.end(); it++)
		{
			ElementArray<Edge> sub = it->getEdges();
			for(ElementArray<Edge>::iterator jt = sub.begin(); jt != sub.end(); jt++)
				if( !jt->GetMarker(mrk) && (mask == 0 || (invert ^ jt->GetMarker(mask))) )
				{
					adjcells.push_back(*jt);
					jt->SetMarker(mrk);
				}
		}
		for(ElementArray<Edge>::iterator it = adjcells.begin(); it != adjcells.end(); it++) it->RemMarker(mrk);
		for(ElementArray<Element>::iterator it = my.begin(); it != my.end(); it++) it->RemMarker(mrk);
		m->ReleaseMarker(mrk);
		return adjcells;
	}

	ElementArray<Face> Element::BridgeAdjacencies2Face(ElementType Bridge, MarkerType mask, bool invert) const
	{
		Mesh * m = GetMeshLink();
		MarkerType mrk = m->CreateMarker();
		ElementArray<Face> adjcells(m);
		ElementArray<Element> adjfaces = getAdjElements(Bridge);
		ElementArray<Element> my = Bridge & GetElementType() ? ElementArray<Element>(m) : getAdjElements(FACE);
		for(ElementArray<Element>::iterator it = my.begin(); it != my.end(); it++) it->SetMarker(mrk);
		for(ElementArray<Element>::iterator it = adjfaces.begin(); it != adjfaces.end(); it++)
		{
			ElementArray<Face> sub = it->getFaces();
			for(ElementArray<Face>::iterator jt = sub.begin(); jt != sub.end(); jt++)
				if( !jt->GetMarker(mrk) && (mask == 0 || (invert ^ jt->GetMarker(mask))) )
				{
					adjcells.push_back(*jt);
					jt->SetMarker(mrk);
				}
		}
		for(ElementArray<Face>::iterator it = adjcells.begin(); it != adjcells.end(); it++) it->RemMarker(mrk);
		for(ElementArray<Element>::iterator it = my.begin(); it != my.end(); it++) it->RemMarker(mrk);
		m->ReleaseMarker(mrk);
		return adjcells;
	}

	ElementArray<Cell> Element::BridgeAdjacencies2Cell(ElementType Bridge, MarkerType mask, bool invert) const
	{
		Mesh * m = GetMeshLink();
		MarkerType mrk = m->CreateMarker();
		ElementArray<Cell> adjcells(m);
		ElementArray<Element> adjfaces = getAdjElements(Bridge);
		ElementArray<Element> my = Bridge & GetElementType() ? ElementArray<Element>(m) : getAdjElements(CELL);
		for(ElementArray<Element>::iterator it = my.begin(); it != my.end(); it++) it->SetMarker(mrk);
		for(ElementArray<Element>::iterator it = adjfaces.begin(); it != adjfaces.end(); it++)
		{
			ElementArray<Cell> sub = it->getCells();
			for(ElementArray<Cell>::iterator jt = sub.begin(); jt != sub.end(); jt++)
				if( !jt->GetMarker(mrk) && (mask == 0 || (invert ^ jt->GetMarker(mask))) )
				{
					adjcells.push_back(*jt);
					jt->SetMarker(mrk);
				}
		}
		for(ElementArray<Cell>::iterator it = adjcells.begin(); it != adjcells.end(); it++) it->RemMarker(mrk);
		for(ElementArray<Element>::iterator it = my.begin(); it != my.end(); it++) it->RemMarker(mrk);
		m->ReleaseMarker(mrk);
		return adjcells;
	}


	Element::Status Element::GetStatus() const
	{
		assert(isValid());
		return GetMeshLink()->GetStatus(GetHandle());
	}
	void Element::SetStatus(Status status) const
	{
		GetMeshLink()->SetStatus(GetHandle(),status);
	}
	Storage::integer & Element::GlobalID() const
	{
		return GetMeshLink()->GlobalID(GetHandle());
	}
	void Element::Centroid  (Storage::real * cnt) const
	{
		GetMeshLink()->GetGeometricData(GetHandle(),CENTROID,cnt);
	}
	void Element::Barycenter(Storage::real * cnt) const
	{
		GetMeshLink()->GetGeometricData(GetHandle(),BARYCENTER,cnt);
	}
	/*
	Element::adj_type &       Element::HighConn() 
	{
		assert(isValid()); 
		return GetMeshLink()->HighConn(GetHandle());
	}
	Element::adj_type &       Element::LowConn () 
	{
		assert(isValid()); 
		return GetMeshLink()->LowConn(GetHandle());
	}
	Element::adj_type const & Element::HighConn() const 
	{
		assert(isValid()); 
		return GetMeshLink()->HighConn(GetHandle());
	}
	Element::adj_type const & Element::LowConn () const 
	{
		assert(isValid()); 
		return GetMeshLink()->LowConn(GetHandle());
	}
	*/
	Element::GeometricType Element::GetGeometricType() const 
	{
		assert(isValid()); 
		return GetMeshLink()->GetGeometricType(GetHandle());
	}
	void Element::SetGeometricType(GeometricType t)
	{
		assert(isValid()); 
		GetMeshLink()->SetGeometricType(GetHandle(),t); 
	}

	Node Element::getAsNode() const {assert(GetElementType() == NODE); return Node(GetMeshLink(),GetHandle());}
	Edge Element::getAsEdge() const {assert(GetElementType() == EDGE); return Edge(GetMeshLink(),GetHandle());}
	Face Element::getAsFace() const {assert(GetElementType() == FACE); return Face(GetMeshLink(),GetHandle());} 
	Cell Element::getAsCell() const {assert(GetElementType() == CELL); return Cell(GetMeshLink(),GetHandle());}
	ElementSet Element::getAsSet() const {assert(GetElementType() == ESET); return ElementSet(GetMeshLink(),GetHandle());}

}
#endif
