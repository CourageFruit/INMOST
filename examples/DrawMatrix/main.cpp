//g++ main.cpp rotate.cpp -L/usr/X11R6/lib -lX11 -lXi -lXmu -lGL -lglut -lGLU ../../inmost.a -O5
// press space - explode mesh to see connection 

#define _CRT_SECURE_NO_WARNINGS
#include "../../inmost.h"
#include "my_glut.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <deque>
#include <stdarg.h>


using namespace INMOST;
int width = 1000, height = 1000;
Solver::Matrix * m = NULL;

int zoom = 200;
int block_size = 0;

//~ Storage::real min = 1e20,max = -1e20;

void printtext(const char * fmt, ...)
{

	unsigned int i;
	char stext[1024];
	va_list ap;
	if (fmt == NULL) return;
	va_start(ap, fmt);
	vsprintf(stext, fmt, ap);
	va_end(ap);
	for (i = 0; i<strlen(stext); i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10,
			stext[i]);
	}
}

class Input
{
public:
	enum InputType { Double, Integer };
private:
	std::string str;
	void * input_link;
	InputType type;
	bool done;


public:
	Input(int * val) { input_link = val; type = Integer; done = false; str = ""; }
	Input(double * val) { input_link = val; type = Double; done = false; str = ""; }
	Input(void * link, InputType type) : input_link(link), type(type) { done = false; str = ""; }
	Input(const Input & other) :input_link(other.input_link), str(other.str), type(other.type), done(other.done) {}
	Input & operator =(Input const & other) { input_link = other.input_link; str = other.str; type = other.type; done = other.done; return *this; }
	~Input() {}
	void KeyPress(char c)
	{
		if (c == 13)
		{

			done = true;
			if (type == Double) *((double *)input_link) = atof(str.c_str());
			else if (type == Integer) *((int *)input_link) = atoi(str.c_str());
			glutPostRedisplay();
		}
		else if (c == 8)
		{
			if (!str.empty()) str.erase(str.size() - 1);
			glutPostRedisplay();
		}
		else if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.' || c == 'e' || c == 'E')
		{
			str += c;
			glutPostRedisplay();
		}
		else if (c == 27)
		{

			done = true;
			glutPostRedisplay();
		}
	}
	bool Done() { return done; }
	void Draw()
	{
		float h = 24.0f / (float)height;

		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
		glVertex3f(-0.99, -0.99, 1);
		glVertex3f(-0.99, -0.99 + h, 1);
		glVertex3f(0.99, -0.99 + h, 1);
		glVertex3f(0.99, -0.99, 1);
		glEnd();
		glColor3f(0, 0, 0);
		glBegin(GL_LINE_LOOP);
		glVertex3f(-0.99, -0.99, 1);
		glVertex3f(-0.99, -0.99 + h, 1);
		glVertex3f(0.99, -0.99 + h, 1);
		glVertex3f(0.99, -0.99, 1);
		glEnd();

		glColor4f(0, 0, 0, 1);
		glRasterPos2d(-0.985, -0.985);
		//printtext(str.c_str());
		printtext("input number (%s): %s", type == Integer ? "integer" : "double", str.c_str());
	}
} *CommonInput = NULL;


class Reorder_ddPQ
{

public:
};

class Reorder_ARMS
{
	interval<INMOST_DATA_ENUM_TYPE, INMOST_DATA_ENUM_TYPE> reorder;
	interval<INMOST_DATA_ENUM_TYPE, std::vector< INMOST_DATA_ENUM_TYPE> > neighbours;
	std::vector< std::pair<INMOST_DATA_ENUM_TYPE, INMOST_DATA_ENUM_TYPE> > node_neighbours;
	std::vector< INMOST_DATA_ENUM_TYPE > block_entries;
	std::vector< INMOST_DATA_ENUM_TYPE > block_ends;
	std::deque<INMOST_DATA_ENUM_TYPE> stack;
	int nblocks, maxblock, sblock;
	INMOST_DATA_ENUM_TYPE mbeg, mend, visited, select;
	Solver::Matrix & A;
	void add_block_entry(INMOST_DATA_ENUM_TYPE ind)
	{
		A[ind].SetMarker();
		block_entries.push_back(ind);
		reorder[ind] = visited++;
		stack.push_back(ind);
	}
	void force_finalize_block()
	{
		for (INMOST_DATA_ENUM_TYPE j = 0; j < block_entries.size(); j++)
		{
			for (INMOST_DATA_ENUM_TYPE k = 0; k < neighbours[block_entries[j]].size(); ++k)
				A[neighbours[block_entries[j]][k]].SetMarker();
		}
		sblock += static_cast<int>(block_entries.size());
		block_ends.push_back(sblock);
		nblocks++;
		stack.clear();
		block_entries.clear();
	}
	bool try_finalize_block()
	{
		if (block_entries.size() >= maxblock)
		{

			/*INMOST_DATA_ENUM_TYPE hide = 0;
			for (INMOST_DATA_ENUM_TYPE j = 0; j < block_entries.size(); j++)
			{
				for (INMOST_DATA_ENUM_TYPE k = 0; k < neighbours[block_entries[j]].size(); ++k)
					if (!A[neighbours[block_entries[j]][k]].GetMarker())
						hide++;
			}
			if (hide < block_entries.size() / 2)*/
			{
				force_finalize_block();
				return true;
			}
		}
		return false;
	}
	INMOST_DATA_ENUM_TYPE GetOrder(INMOST_DATA_ENUM_TYPE ind)
	{
		return static_cast<Storage::enumerator>(neighbours[ind].size());
		INMOST_DATA_ENUM_TYPE ret = 0, k;
		for (k = 0; k < neighbours[ind].size(); k++)
		if (!A[neighbours[ind][k]].GetMarker())
			ret++;
		return ret;
	}
public:
	INMOST_DATA_ENUM_TYPE GetBlockNumber()
	{
		return nblocks;
	}
	INMOST_DATA_ENUM_TYPE GetMatrixPartSize()
	{
		return sblock;
	}
	void clear()
	{
		for (INMOST_DATA_ENUM_TYPE k = mbeg; k < mend; k++)
			reorder[k] = k;
		sblock = 0;
		nblocks = 0;
	}
	Reorder_ARMS(Solver::Matrix * m, INMOST_DATA_ENUM_TYPE _mbeg, INMOST_DATA_ENUM_TYPE _mend) : A(*m)
	{
		nblocks = 0;
		maxblock = 120;
		sblock = 0;
		mbeg = _mbeg;
		mend = _mend;
		//m->GetInterval(mbeg, mend);
		reorder.set_interval_beg(mbeg);
		reorder.set_interval_end(mend);
		neighbours.set_interval_beg(mbeg);
		neighbours.set_interval_end(mend);

		for (INMOST_DATA_ENUM_TYPE k = mbeg; k < mend; ++k)
		{
			for (Solver::Row::iterator it = A[k].Begin(); it != A[k].End(); ++it)
			if ( it->first >= mbeg && it->first < mend)
				neighbours[it->first].push_back(k);
		}
		for (INMOST_DATA_ENUM_TYPE k = mbeg; k < mend; ++k)
		{
			for (Solver::Row::iterator it = A[k].Begin(); it != A[k].End(); ++it)
			if (it->first >= mbeg && it->first < mend)
				neighbours[k].push_back(it->first);

			std::sort(neighbours[k].begin(), neighbours[k].end());
			neighbours[k].resize(std::unique(neighbours[k].begin(), neighbours[k].end()) - neighbours[k].begin());
		}
		clear();
	}
	INMOST_DATA_ENUM_TYPE position(INMOST_DATA_ENUM_TYPE index) { return reorder[index]; }
	void compute(int _maxblock)
	{
		//maxblock = (mend - mbeg) * 0.85 / 4;//_maxblock;
		
		maxblock = _maxblock;
		std::cout << "Block size: " << maxblock << std::endl;
		sblock = 0;
		nblocks = 0;
		std::fill(reorder.begin(), reorder.end(), ENUMUNDEF);
		visited = mbeg;
		node_neighbours.clear();
		stack.clear();
		block_ends.clear();
		block_entries.clear();
		block_ends.push_back(0);
		while (true)
		{
			select = ENUMUNDEF;
			for (INMOST_DATA_ENUM_TYPE k = mbeg; k < mend; ++k)
			{
				if (!A[k].GetMarker())
				{
					if (select == ENUMUNDEF || GetOrder(k) < GetOrder(select))
						select = k;
				}
			}
			if (select == ENUMUNDEF) break;

			add_block_entry(select);
			try_finalize_block();

			while (!stack.empty())
			{
				if (visited % 50 == 0)
				{
					std::cout << visited * 100.0 / (mend - mbeg) << "% " << nblocks << " blocks\r";
					std::cout.flush();
				}
				select = stack.front();
				stack.pop_front();
				for (INMOST_DATA_ENUM_TYPE k = 0; k != neighbours[select].size(); ++k)
				{
					INMOST_DATA_ENUM_TYPE j = neighbours[select][k];
					if (!A[j].GetMarker())
					{
						node_neighbours.push_back(std::pair<INMOST_DATA_ENUM_TYPE, INMOST_DATA_ENUM_TYPE>(GetOrder(j), j));
						//A[j].SetMarker();
					}
				}
				//std::sort(node_neighbours.rbegin(), node_neighbours.rend());
				std::sort(node_neighbours.begin(), node_neighbours.end());
				for (INMOST_DATA_ENUM_TYPE k = 0; k < node_neighbours.size(); ++k)
				{
					add_block_entry(node_neighbours[k].second);
					if (try_finalize_block()) break;
				}
				node_neighbours.clear();
			}
		}
		if (block_entries.size() > maxblock * 0.5)
			force_finalize_block();
		std::cout << "Number of blocks: " << nblocks << std::endl;
		std::cout << "Restructured matrix size: " << sblock << std::endl;
		std::cout << "Left untouched: " << mend - mbeg - visited << std::endl;
		for (INMOST_DATA_ENUM_TYPE k = mbeg; k < mend; ++k)
		{
			A[k].RemMarker();
			if (reorder[k] == ENUMUNDEF)
				reorder[k] = visited++;
		}
		std::cout << "Completed ordering: " << visited << std::endl;
		//calculate average density
		{
			INMOST_DATA_REAL_TYPE nnzall = 0.0;
			std::vector<INMOST_DATA_REAL_TYPE> nnz(nblocks,0.0);
			for (INMOST_DATA_ENUM_TYPE i = mbeg; i < mend; i++)
			for (Solver::Row::iterator r = A[i].Begin(); r != A[i].End(); ++r)
			{
				INMOST_DATA_ENUM_TYPE ki = position(i), kj = position(r->first), p;
				std::vector<INMOST_DATA_ENUM_TYPE>::iterator qi = std::lower_bound(block_ends.begin(), block_ends.end(), ki);
				std::vector<INMOST_DATA_ENUM_TYPE>::iterator qj = std::lower_bound(block_ends.begin(), block_ends.end(), kj);
				if (qi == qj && qi != block_ends.end())
				{
					p = qi - block_ends.begin();
					if (p < static_cast<unsigned>(nblocks))
						nnz[p] += 1.0;
				}
			}
			for (INMOST_DATA_ENUM_TYPE k = 0; k < static_cast<unsigned>(nblocks); ++k)
			{
				nnz[k] /= static_cast<INMOST_DATA_REAL_TYPE>((block_ends[k + 1] - block_ends[k])*(block_ends[k + 1] - block_ends[k]));
				nnzall += nnz[k];
			}
			std::cout << "average block density: " << nnzall / static_cast<INMOST_DATA_REAL_TYPE>(nblocks) << std::endl;
		}
	}
} * ord;


void set_output_matrix()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, m->Size(),
		0, m->Size(),
		-1, 1);
	glMatrixMode(GL_MODELVIEW);
}

void set_output_uid()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1, 1, -1, 1, -1, 1);
	glMatrixMode(GL_MODELVIEW);
}

void reshape(int w, int h)
{
	if( m != NULL )
	{
		set_output_matrix();
		glViewport(0, 0, w, h);
	}
}



void keyboard(unsigned char key, int x, int y)
{
	if (CommonInput != NULL)
	{
		CommonInput->KeyPress(key);
		return;
	}
	if( key == 27 )
	{
		delete m;
		exit(-1);
	}
	if( key == '+' || key == '=' )
	{
		zoom++;
		
	}
	if( key == '-' )
	{
		if( zoom > 1 ) 
		{
			zoom--;
			
		}
	}
	if (key == 'c') ord->clear();
	if (key == 'r')
	{
		CommonInput = new Input(&block_size);
	}
	glutPostRedisplay();
}

void DrawEntry(int i, int j)//, Storage::real r)
{
	//~ glColor3f(r,0.0,1.0-r);
	glVertex2i(i-(zoom-1),j-(zoom-1)-1);
	glVertex2i(i+zoom,j-(zoom-1)-1);
	glVertex2i(i+zoom,j+zoom-1);
	glVertex2i(i-(zoom-1),j+zoom-1);
}


interval<int,double> row_sum;

void draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	set_output_matrix();

	/*
	glColor3f(0, 1, 1);
	glBegin(GL_LINES);
	glVertex2i(ord->GetMatrixPartSize(), 0);
	glVertex2i(ord->GetMatrixPartSize(), m->Size());

	glVertex2i(0, m->Size() - ord->GetMatrixPartSize());
	glVertex2i(m->Size(), m->Size() - ord->GetMatrixPartSize());
	glEnd();
	*/

	glColor3f(0,0,0);
	glBegin(GL_QUADS);
	for(Solver::Matrix::iterator it = m->Begin(); it != m->End(); ++it)
		for(Solver::Row::iterator jt = it->Begin(); jt != it->End(); ++jt)
			if( jt->first != it - m->Begin() )
			//DrawEntry(ord->position((it - m->Begin())), m->Size() - ord->position(jt->first));//, sqrt((jt->second-min)/(max-min)));
			DrawEntry((it - m->Begin()), m->Size() - jt->first);//, sqrt((jt->second-min)/(max-min)));
	glEnd();

	
	/*
	glColor3f(0.0, 1.0, 0);
	glBegin(GL_QUADS);
	for (Solver::Matrix::iterator it = m->Begin(); it != m->End(); ++it)
	{
		int ind = it - m->Begin();
		if (fabs((*it)[ind]) > row_sum[it-m->Begin()]) //DrawEntry(ord->position((it - m->Begin())), m->Size() - ord->position(ind));
			DrawEntry((it - m->Begin()), m->Size() - ind);
	}
	glEnd();
	*/

	
	glBegin(GL_QUADS);
	for (Solver::Matrix::iterator it = m->Begin(); it != m->End(); ++it)
	{
		
		int ind = it - m->Begin();
		double t = fabs((*it)[ind]) / row_sum[ind];
		//if (fabs((*it)[ind]) < row_sum[ind]) //DrawEntry(ord->position((it - m->Begin())), m->Size() - ord->position(ind));
		glColor3f(0.0, t, 1.0-t);
			DrawEntry(ind, m->Size() - ind);
	}
	glEnd();

	zoom += 1;
	glColor3f(1.0, 0, 0);
	glBegin(GL_QUADS);
	for (Solver::Matrix::iterator it = m->Begin(); it != m->End(); ++it)
	{
		int ind = it - m->Begin();
		if (fabs((*it)[ind]) < 1e-9) //DrawEntry(ord->position((it - m->Begin())), m->Size() - ord->position(ind));
			DrawEntry((it - m->Begin()), m->Size() - ind);
	}
	glEnd();
	zoom -= 1;

	if (CommonInput != NULL)
	{
		glDisable(GL_DEPTH_TEST);
		glLoadIdentity();
		set_output_uid();
		CommonInput->Draw();
		if (CommonInput->Done())
		{
			delete CommonInput;
			CommonInput = NULL;
			ord->compute(block_size);
			glutPostRedisplay();
		}
		glEnable(GL_DEPTH_TEST);
	}

	glutSwapBuffers();
}



int main(int argc, char ** argv)
{
	if( argc < 2 )
	{
		std::cout << "usage: " << argv[0] << " matrix.mtx " << std::endl;
		return -1;
	}
	Solver::Initialize(&argc,&argv,NULL);
	m = new Solver::Matrix();
	m->Load(argv[1]);
	//ord = new Reorder_ARMS(m,0,m->Size());
	std::cout << "Matrix size: " << m->Size() << std::endl;
	INMOST_DATA_ENUM_TYPE nnz = 0, nnzrow;
	
	row_sum.set_interval_beg(0);
	row_sum.set_interval_end(m->Size());
	std::fill(row_sum.begin(),row_sum.end(),0.0);

	for (Solver::Matrix::iterator it = m->Begin(); it != m->End(); ++it)
	{
		nnz += it->Size();

		nnzrow = 0;
		for(Solver::Row::iterator jt = it->Begin(); jt != it->End(); ++jt)
		{
			nnzrow++;
			row_sum[it-m->Begin()] += fabs(jt->second);
		}
		row_sum[it-m->Begin()] /= (double)nnzrow;
	}

	

	std::cout << "Nonzeros: " << nnz << std::endl;
	
	zoom = m->Size() / 1000;
	
	//~ for(Solver::Matrix::iterator it = m->Begin(); it != m->End(); ++it)
		//~ for(Solver::Row::iterator jt = it->Begin(); jt != it->End(); ++jt)
		//~ {
			//~ if(jt->second < min ) 
			//~ {
				//~ min = jt->second;
				//~ std::cout << (it-m->Begin()) << " " << jt->first << " " << jt->second << std::endl;
			//~ }
			//~ if(jt->second > max ) 
			//~ {
				//~ max = jt->second;
				//~ std::cout << (it-m->Begin()) << " " << jt->first << " " << jt->second << std::endl;
			//~ }
		//~ }
	//~ std::cout << "max: " << max << " min: " << min << std::endl;
	Solver::Finalize();
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(width, height);
	glutInitWindowPosition (5, 5);
	glutCreateWindow(argv[1]);

	

	glClearColor (1.0f, 1.0f, 1.0f, 1.f);
	glutDisplayFunc(draw);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	

	glutPostRedisplay();
	glutMainLoop();
}
