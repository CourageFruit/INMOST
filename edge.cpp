#include "inmost.h"
#if defined(USE_MESH)
namespace INMOST
{
	
	Node Edge::getBeg() const 
	{
		assert(GetHandleElementType(GetHandle())==EDGE);
		Mesh * m = GetMeshLink();
		if( !GetMeshLink()->HideMarker() )
		{
			adj_type const & lc = m->LowConn(GetHandle());
			if( lc.empty() )
				return Node(m,InvalidHandle());
			return Node(m,lc.front());
		}
		else
		{
			adj_type const & lc = m->LowConn(GetHandle());
			if( !lc.empty() )
			{
				enumerator i = ENUMUNDEF;
				MarkerType hm = m->HideMarker();
				i = m->getNext(lc.data(),static_cast<enumerator>(lc.size()),i,hm);
				if( i != lc.size() ) return Node(m,lc[i]);
			}
			return Node(m,InvalidHandle());
		}
	}
	Node Edge::getEnd() const 
	{
		assert(GetHandleElementType(GetHandle())==EDGE);
		Mesh * m = GetMeshLink();
		if( !m->HideMarker() )
		{
			adj_type const & lc = m->LowConn(GetHandle());
			if( lc.size() < 2 )
				return Node(m,InvalidHandle());
			return Node(m,lc.back());
		}
		else
		{
			adj_type const & lc = m->LowConn(GetHandle());
			if( !lc.empty() )
			{
				enumerator i = ENUMUNDEF;
				MarkerType hm = m->HideMarker();
				i = m->getNext(lc.data(),static_cast<enumerator>(lc.size()),i,hm);
				i = m->getNext(lc.data(),static_cast<enumerator>(lc.size()),i,hm);
				if( i != lc.size() ) return Node(m,lc[i]);
			}
			return Node(m,InvalidHandle());
		}
	}
	
	ElementArray<Node> Edge::getNodes() const
	{
		assert(GetHandleElementType(GetHandle())==EDGE);
		Mesh * m = GetMeshLink();
		if( !m->HideMarker() )
		{
			adj_type & lc = m->LowConn(GetHandle());
			return ElementArray<Node>(m,lc.data(),lc.data()+lc.size());
		}
		else
		{
			MarkerType hm = m->HideMarker();
			ElementArray<Node> aret(m);
			adj_type const & lc = m->LowConn(GetHandle());
			for(adj_type::size_type it = 0; it < lc.size(); ++it)
				if( !m->GetMarker(lc[it],hm) ) aret.push_back(lc[it]);
			return aret;
		}
	}

	ElementArray<Node> Edge::getNodes(MarkerType mask, bool invert) const
	{
		assert(GetHandleElementType(GetHandle())==EDGE);
		Mesh * m = GetMeshLink();
		ElementArray<Node> aret(m);
		if( !m->HideMarker() )
		{	
			adj_type const & lc = m->LowConn(GetHandle());
			for(adj_type::size_type it = 0; it < lc.size(); ++it)
				if( invert ^ m->GetMarker(lc[it],mask) ) aret.push_back(lc[it]);
		}
		else
		{
			MarkerType hm = m->HideMarker();
			adj_type const & lc = m->LowConn(GetHandle());
			for(adj_type::size_type it = 0; it < lc.size(); ++it)
				if( (invert ^ m->GetMarker(lc[it],mask)) && !m->GetMarker(lc[it],hm) ) aret.push_back(lc[it]);
		}
		return aret;
	}
	
	ElementArray<Face> Edge::getFaces() const
	{
		assert(GetHandleElementType(GetHandle())==EDGE);
		Mesh * m = GetMeshLink();
		if( !m->HideMarker() )
		{
			adj_type const & hc = m->HighConn(GetHandle());
			return ElementArray<Face>(m,hc.data(),hc.data()+hc.size());
		}
		else
		{
			MarkerType hm = m->HideMarker();
			ElementArray<Face> aret(m);
			adj_type const & hc = m->HighConn(GetHandle());
			for(adj_type::size_type it = 0; it < hc.size(); ++it)
				if( !m->GetMarker(hc[it],hm) ) aret.push_back(hc[it]);
			return aret;
		}
	}


	ElementArray<Face> Edge::getFaces(MarkerType mask, bool invert) const
	{
		assert(GetHandleElementType(GetHandle())==EDGE);
		Mesh * m = GetMeshLink();
		ElementArray<Face> aret(m);
		if( !m->HideMarker() )
		{
			adj_type const & hc = m->HighConn(GetHandle());
			for(adj_type::size_type it = 0; it < hc.size(); ++it)
				if( (invert ^ m->GetMarker(hc[it],mask)) ) aret.push_back(hc[it]);
		}
		else
		{
			MarkerType hm = GetMeshLink()->HideMarker();
			adj_type const & hc = m->HighConn(GetHandle());
			for(adj_type::size_type it = 0; it < hc.size(); ++it)
				if( (invert ^ m->GetMarker(hc[it],mask)) && !m->GetMarker(hc[it],hm) ) aret.push_back(hc[it]);
		}
		return aret;
	}

	ElementArray<Cell> Edge::getCells() const
	{
		assert(GetHandleElementType(GetHandle())==EDGE);
		Mesh * m = GetMeshLink();
		ElementArray<Cell> aret(m);
		MarkerType mrk = m->CreateMarker();
		if( !m->HideMarker() )
		{
			adj_type const & hc = m->HighConn(GetHandle());
			for(adj_type::size_type it = 0; it < hc.size(); it++) //faces
			{
				adj_type const & ihc = m->HighConn(hc[it]);
				for(adj_type::size_type jt = 0; jt < ihc.size(); jt++) //cels
					if( !m->GetMarker(ihc[jt],mrk) )
					{
						aret.push_back(ihc[jt]);
						m->SetMarker(ihc[jt],mrk);
					}
			}
		}
		else
		{
			MarkerType hm = m->HideMarker();
			adj_type const & hc = m->HighConn(GetHandle());
			for(adj_type::size_type it = 0; it < hc.size(); it++) if( !m->GetMarker(hc[it],hm) ) //faces
			{
				adj_type const & ihc = m->HighConn(hc[it]);
				for(adj_type::size_type jt = 0; jt < ihc.size(); jt++) if( !m->GetMarker(ihc[jt],hm) ) //cels
					if( !m->GetMarker(ihc[jt],mrk) )
					{
						aret.push_back(ihc[jt]);
						m->SetMarker(ihc[jt],mrk);
					}
			}
		}
		for(ElementArray<Cell>::size_type it = 0; it < aret.size(); it++) m->RemMarker(aret.at(it),mrk);
		m->ReleaseMarker(mrk);
		return aret;
	}

	ElementArray<Cell> Edge::getCells(MarkerType mask, bool invert) const
	{
		assert(GetHandleElementType(GetHandle())==EDGE);
		Mesh * m = GetMeshLink();
		ElementArray<Cell> aret(m);
		
		MarkerType mrk = m->CreateMarker();
		if( !GetMeshLink()->HideMarker() )
		{
			adj_type const & hc = m->HighConn(GetHandle());
			for(adj_type::size_type it = 0; it < hc.size(); it++) //faces
			{
				adj_type const & ihc = m->HighConn(hc[it]);
				for(adj_type::size_type jt = 0; jt < ihc.size(); jt++) //cels
					if( (invert ^ m->GetMarker(ihc[jt],mask)) && !m->GetMarker(ihc[jt],mrk) )
					{
						aret.push_back(ihc[jt]);
						m->SetMarker(ihc[jt],mrk);
					}
			}
		}
		else
		{
			MarkerType hm = m->HideMarker();
			adj_type const & hc = m->HighConn(GetHandle());
			for(adj_type::size_type it = 0; it < hc.size(); it++) if( !m->GetMarker(hc[it],hm) ) //faces
			{
				adj_type const & ihc = m->HighConn(hc[it]);
				for(adj_type::size_type jt = 0; jt < ihc.size(); jt++) if( !m->GetMarker(ihc[jt],hm) ) //cels
					if( (invert ^ m->GetMarker(ihc[jt],mask)) && !m->GetMarker(ihc[jt],mrk) )
					{
						aret.push_back(ihc[jt]);
						m->SetMarker(ihc[jt],mrk);
					}
			}
		}
		for(ElementArray<Cell>::size_type it = 0; it < aret.size(); it++) 
			m->RemMarker(aret.at(it),mrk);
		m->ReleaseMarker(mrk);
		return aret;
	}
}

#endif
