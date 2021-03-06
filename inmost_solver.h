
#ifndef INMOST_SOLVER_INCLUDED
#define INMOST_SOLVER_INCLUDED


#include "inmost_common.h"
//#include "solver_prototypes.hpp"

#define DEFAULT_ADDITIVE_SCHWARTZ_OVERLAP             1
#define DEFAULT_ABSOLUTE_TOLERANCE                    1.0e-5
#define DEFAULT_RELATIVE_TOLERANCE                    1.0e-12
#define DEFAULT_DIVERGENCE_TOLERANCE                  1.0e+100
#define DEFAULT_MAXIMUM_ITERATIONS                    2500
#define DEFAULT_SOLVER_GMRES_SUBSTEPS                 2
#define DEFAULT_PRECONDITIONER_DROP_TOLERANCE         0.005
#define DEFAULT_PRECONDITIONER_REUSE_TOLERANCE        0.00005
#define DEFAULT_PRECONDITIONER_FILL_LEVEL             3
#define DEFAULT_PRECONDITIONER_DDPQ_TOLERANCE         0.75
#define DEFAULT_PRECONDITIONER_REORDER_NONZEROS       1
#define DEFAULT_PRECONDITIONER_RESCALE_ITERS          6
#define DEFAULT_PRECONDITIONER_CONDITION_ESTIMATION   1
#define DEFAULT_PRECONDITIONER_ADAPT_DDPQ_TOLERANCE   1

#if defined(USE_SOLVER)
namespace INMOST
{
	/// Main class to set and solve linear system.
	/// Solver class is used to set the coefficient Matrix, the right-hand side Vector
	/// and the initial guess Vector, construct the preconditioner and Solve
	/// the linear system.
	///
	/// Formally, Solver class is independent of INMOST::Mesh class.
	/// @see Solver::Matrix
	/// @see Solver::Vector
	/// @see Solver::Solve
	class Solver
	{
	private:
		static INMOST_MPI_Type RowEntryType; //prepared in Initialize
	public:
		/// Type of the Solver can be currently used in this version of INMOST.
		enum Type
		{
			INNER_ILU2,     ///< inner Solver based on BiCGStab(L) solver with second order ILU factorization as preconditioner.
			INNER_DDPQILUC,   ///< inner Solver based on BiCGStab(L) solver with second order Crout-ILU with inversed-based condition estimation and unsymmetric reordering for diagonal dominance as preconditioner.
			INNER_MPTILUC,   ///< inner Solver based on BiCGStab(L) solver with second order Crout-ILU with inversed-based condition estimation and maximum product transversal reordering as preconditioner.
			INNER_MPTILU2,   ///< inner Solver based on BiCGStab(L) solver with second order ILU and maximum product transversal reordering as preconditioner.
			Trilinos_Aztec, ///< external Solver AztecOO from Trilinos package
			Trilinos_Belos, ///< external Solver Belos from Trilinos package, currently without preconditioner
			Trilinos_ML,    ///< external Solver AztecOO with ML preconditioner
			Trilinos_Ifpack,///< external Solver AztecOO with Ifpack preconditioner
			PETSc,          ///< external Solver PETSc, @see http://www.mcs.anl.gov/petsc/.
			ANI             ///< external Solver from ANI3D based on ILU2 (sequential Fortran version).
		};

		static INMOST_MPI_Type & GetRowEntryType() {return RowEntryType;}
		
		//solver.cpp::::::::::::::::::::::::::::::::::::::::::::::::::::
	public:
		class Matrix;
		class Vector;

		/// Base class for low level operations with objects of Solver class.
		class OrderInfo
		{
		private:
			typedef std::vector<INMOST_DATA_ENUM_TYPE> storage_type;
			storage_type global_to_proc; //stores ends of all non-overlapping intervals of elements, owned by this processor
			storage_type global_overlap; //stores pairs: [begin,end) of overlapping intervals of rows
			std::vector<INMOST_DATA_ENUM_TYPE> vector_exchange_recv, vector_exchange_send;
			std::vector<INMOST_DATA_REAL_TYPE> send_storage, recv_storage;
			std::vector<INMOST_MPI_Request> send_requests, recv_requests;
			std::vector<INMOST_DATA_ENUM_TYPE> extended_indexes;

			//remote indexes
			INMOST_DATA_ENUM_TYPE local_vector_begin, local_vector_end;
			INMOST_DATA_ENUM_TYPE initial_matrix_begin, initial_matrix_end; //local interval of matrix
			INMOST_DATA_ENUM_TYPE local_matrix_begin, local_matrix_end; //local interval of matrix

			bool have_matrix;
			INMOST_MPI_Comm comm;
			int rank,size;
		public:
			void Clear();
			/// Return true if Matrix data have already been specified.
			bool & HaveMatrix() { return have_matrix; }
			OrderInfo();
			OrderInfo(const OrderInfo & other);
			OrderInfo & operator =(OrderInfo const & other);
			~OrderInfo();
			/// Prepare parallel state of the Matrix with specified overlap size.
			/// This state of the matrix can be used, for instance, to construct
			/// the preconditioner for Additive Swartz method.
			/// @param m Matrix to be expanded.
			/// @param overlap Overlap size, viz. the number of overlap layers.
			void PrepareMatrix(Matrix & m, INMOST_DATA_ENUM_TYPE overlap);
			/// Restore initial nonparallel state of the Matrix with no overlap.
			void RestoreMatrix(Matrix & m);
			/// Prepare parallel state of the Vector.
			void PrepareVector(Vector & v) const;
			/// Restore initial nonparallel state of the Vector.
			void RestoreVector(Vector & v) const;
			/// Retrieve the processor number by binary search for the specified global index.
			INMOST_DATA_ENUM_TYPE GetProcessor(INMOST_DATA_ENUM_TYPE gind) const; //retrieve processor by binary search in global_to_proc
			void      GetOverlapRegion(INMOST_DATA_ENUM_TYPE proc, INMOST_DATA_ENUM_TYPE & mbeg, INMOST_DATA_ENUM_TYPE & mend) const;
			/// Get the local index region for the specified process.
			void        GetLocalRegion(INMOST_DATA_ENUM_TYPE proc, INMOST_DATA_ENUM_TYPE & mbeg, INMOST_DATA_ENUM_TYPE & mend) const;
			/// Get the local index region for the current process.
			void       GetVectorRegion(INMOST_DATA_ENUM_TYPE & mbeg, INMOST_DATA_ENUM_TYPE & mend) const {mbeg = local_vector_begin; mend = local_vector_end;}
			/// Get the rank of the current communicator, i.e. the current process index.
			INMOST_DATA_ENUM_TYPE GetRank() const {return rank;}
			/// Get the size of the current communicator, i.e. the total number of processes used.
			INMOST_DATA_ENUM_TYPE GetSize() const {return size;}
			/// Update the shared data in parallel vector.
			void                  Update    (Vector & x); // update parallel vector
			/// Sum shared values in parallel vector.
			void                  Accumulate(Vector & x); // sum shared values in parallel vector
			/// Get the sum of num elements of real array on all processes.
			void                  Integrate(INMOST_DATA_REAL_TYPE * inout, INMOST_DATA_ENUM_TYPE num) const;
			/// Get the communicator which the solver is associated with.
			INMOST_MPI_Comm         GetComm() const {return comm;}
			// Access to arrays below allows to organize manual exchange
			INMOST_MPI_Request    * GetSendRequests() {assert(!send_requests.empty()); return &send_requests[0];}
			INMOST_MPI_Request    * GetRecvRequests() {assert(!recv_requests.empty()); return &recv_requests[0];}
			INMOST_DATA_ENUM_TYPE   GetSendRequestsSize() {return static_cast<INMOST_DATA_ENUM_TYPE>(send_requests.size());}
			INMOST_DATA_ENUM_TYPE   GetRecvRequestsSize() {return static_cast<INMOST_DATA_ENUM_TYPE>(recv_requests.size());}
			INMOST_DATA_ENUM_TYPE * GetSendExchangeArray() {assert(!vector_exchange_send.empty()); return &vector_exchange_send[0];}
			INMOST_DATA_ENUM_TYPE   GetSendExchangeSize() {return static_cast<INMOST_DATA_ENUM_TYPE>(send_storage.size());}
			INMOST_DATA_ENUM_TYPE * GetRecvExchangeArray() {assert(!vector_exchange_recv.empty()); return &vector_exchange_recv[0];}
			INMOST_DATA_ENUM_TYPE   GetRecvExchangeSize() {return static_cast<INMOST_DATA_ENUM_TYPE>(recv_storage.size());}
			//for debug
			//~ void                 BeginSequentialCode() {for(int i = 0; i < rank; i++) MPI_Barrier(comm);}
			//~ void                   EndSequentialCode() {for(int i = rank; i < size; i++) MPI_Barrier(comm);}

			// Get the scalar product of the specified interval of the distributed vector.
			// Conflicts with OpenMP, should not be used in future
			//void ScalarProd(Vector const & left, Vector const & right, INMOST_DATA_ENUM_TYPE index_begin, INMOST_DATA_ENUM_TYPE index_end, INMOST_DATA_REAL_TYPE & sum) const;
		};

		/// Distributed vector class.
		/// This class can be used to store both local and distributed dense data of real type.
		/// For example, to form the right-hand side or initial guess to the solution.
		/// @see Solver::Solve
		class Vector
		{
		public:
			typedef interval<INMOST_DATA_ENUM_TYPE,INMOST_DATA_REAL_TYPE> Entries;
			typedef Entries::iterator iterator;
			typedef Entries::const_iterator const_iterator;
		private:
			INMOST_MPI_Comm comm;
			Entries data;
			std::string name;
			bool is_parallel;
		public:
			/// Main constructor of the Vector class.
			/// @param _name Name of the vector, empty string by default.
			/// @param start Start of the local data interval.
			/// @param end End of the local data interval.
			/// @param _comm Communicator for parallel data exchanges, MPI_COMM_WORLD by default.
			Vector(std::string _name = "", INMOST_DATA_ENUM_TYPE start = 0, INMOST_DATA_ENUM_TYPE end = 0, INMOST_MPI_Comm _comm = INMOST_MPI_COMM_WORLD);
			Vector(const Vector & other);
			Vector & operator =(Vector const & other);
			~Vector();
			/// Return reference to i-th element of the vector.
			INMOST_DATA_REAL_TYPE & operator [](INMOST_DATA_ENUM_TYPE i) {return data[i];}
			/// Return i-th element of the vector.
			INMOST_DATA_REAL_TYPE   operator [](INMOST_DATA_ENUM_TYPE i) const {return data[i];}
			/// Return the global size of the vector.
			INMOST_DATA_ENUM_TYPE  Size() const { return static_cast<INMOST_DATA_ENUM_TYPE>(data.size()); }
			iterator             Begin() {return data.begin();}
			const_iterator       Begin() const {return data.begin();}
			iterator             End() {return data.end();}
			const_iterator       End() const {return data.end();}
			bool                 Empty() const {return data.empty();}
			/// Set the start and the end of the distributed vector interval.
			void                 SetInterval(INMOST_DATA_ENUM_TYPE   start, INMOST_DATA_ENUM_TYPE   end)       {assert(start<=end); data.set_interval_beg(start); data.set_interval_end(end);}
			/// Get the start and the end of the distributed vector interval.
			void                 GetInterval(INMOST_DATA_ENUM_TYPE & start, INMOST_DATA_ENUM_TYPE & end) const {start = data.get_interval_beg(); end = data.get_interval_end();}
			void                 ShiftInterval(INMOST_DATA_ENUM_TYPE shift) {data.shift_interval(shift);}
			/// Get the first index of the distributed vector interval.
			INMOST_DATA_ENUM_TYPE  GetFirstIndex() const {return data.get_interval_beg();}
			/// Get the communicator which the vector is associated with.
			INMOST_MPI_Comm        GetCommunicator() const {return comm;}

			void Swap(Solver::Vector & other) {data.swap(other.data); name.swap(other.name); std::swap(is_parallel,other.is_parallel); std::swap(comm,other.comm);}


			/// Save the distributed vector to a single data file using parallel MPI I/O.
			void                 Save(std::string file);
			/// Load the vector from a single data file using the specified interval.
			/// If interval is not specified, then it will be automatically constructed,
			/// with the about equal block size (the last block may has larger dimension).
			void                 Load(std::string file, INMOST_DATA_ENUM_TYPE mbeg = ENUMUNDEF, INMOST_DATA_ENUM_TYPE mend = ENUMUNDEF);
			
			bool                 & isParallel() {return is_parallel;}
			/// Get the vector name specified in the main constructor.
			std::string          GetName() {return name;}

			/// Clear all data of the current vector.
			void Clear() {data.clear();}
			//~ friend class Solver;
		};



		/// Class to store the sparse matrix row.
		class Row 
		{
		public:
			
			/// Entry of the sparse matrix row.
			typedef struct entry_s 
			{
				INMOST_DATA_ENUM_TYPE first;  ///< the column number of the row element.
				INMOST_DATA_REAL_TYPE second; ///< the real value of the row element.
				//entry_s() :first(0), second(0.0) {}
				//entry_s(const entry_s & other) :first(other.first), second(other.second) {}//{std::cout << __FUNCTION__ << " " << first << " " << second << std::endl;}
				//entry_s(INMOST_DATA_ENUM_TYPE first, INMOST_DATA_REAL_TYPE second):first(first),second(second){}
				//entry_s & operator =(entry_s const & other) {first = other.first, second = other.second; return *this;}
				bool operator < (const entry_s & other) const { return first < other.first || (first == other.first && second < other.second); }
			} entry;
			__INLINE static entry make_entry(INMOST_DATA_ENUM_TYPE ind, INMOST_DATA_REAL_TYPE val)
			{
				entry ret;
				ret.first = ind;
				ret.second = val;
				return ret;
			}

		private:
			typedef dynarray<entry,16> Entries; //replace later with more memory-efficient chunk_array, with first chunk in stack
			//typedef array<entry> Entries;
			//typedef std::vector<entry> Entries;
			//typedef sparse_data<INMOST_DATA_ENUM_TYPE,INMOST_DATA_REAL_TYPE> Entries;
			//typedef Entries::pair entry; //for sparse_data
			
		public:
			typedef Entries::iterator iterator;
			typedef Entries::const_iterator const_iterator;
			typedef Entries::reverse_iterator reverse_iterator;
			typedef Entries::const_reverse_iterator const_reverse_iterator;

			bool modified_pattern; //remove this in future
		private:
#if defined(USE_OMP)
			omp_lock_t lock;
#endif
			bool marker;
			Entries data;
		public:
			void Report() {data.report_addr();}
			void SetMarker() { marker = true; }
			void RemMarker() { marker = false; }
			bool GetMarker() { return marker; }
			Row() :data() 
			{
#if defined(USE_OMP)
				omp_init_lock(&lock);
#endif
				modified_pattern = marker = false;
			}
			Row(const Row & other) :marker(other.marker),data(other.data) 
			{ 
				//std::cout << __FUNCTION__ << " ";
				//for(iterator it = Begin(); it != End(); ++it) std::cout << it->first << "," << it->second << " ";
				//std::cout << std::endl;
#if defined(USE_OMP)
				omp_init_lock(&lock);
#endif
				modified_pattern = other.modified_pattern; 
			}
			Row(entry * pbegin, entry * pend) :data(pbegin, pend) 
			{ 
#if defined(USE_OMP)
				omp_init_lock(&lock);
#endif
				modified_pattern = true; marker = false; 
			}
			void Lock() 
			{
#if defined(USE_OMP)
				omp_set_lock(&lock); 
#endif
			}
			void Unlock() 
			{ 
#if defined(USE_OMP)
				omp_unset_lock(&lock); 
#endif
			}
			~Row() {}
			Row & operator = (Row const & other) { data = other.data; marker = other.marker; return *this; }
			/// The operator [] used to fill the sparse matrix row, but not to access individual elements of the row.
			INMOST_DATA_REAL_TYPE & operator [](INMOST_DATA_ENUM_TYPE i) // use to fill matrix, not to access individual elements
			{
				//for sparse_data type
				//return data[i];
				//for dynarray or array
				
				for(Entries::size_type it = 0; it < data.size(); ++it)
					if( data[it].first == i ) return data[it].second;
				entry new_entry;
				new_entry.first = i;
				new_entry.second = 0;
				data.push_back(new_entry);
				modified_pattern = true;
				return data.back().second;
				
			}
			/// The operator [] used to access individual elements of the row.
			INMOST_DATA_REAL_TYPE operator[](INMOST_DATA_ENUM_TYPE i) const
			{
				//for sparse data type
				//return data[i];

				for (Entries::size_type it = 0; it < data.size(); ++it) if (data[it].first == i) return data[it].second;

				//you should not come here
				assert(false);
				return 1.0e20;
			}
			//void           Reserve(INMOST_DATA_ENUM_TYPE num) { data.reserve(num);}
			/// Clear all data of the current row.
			void                    Clear() { data.clear(); }
			void                    Swap(Solver::Row & other) { data.swap(other.data); bool tmp = marker; marker = other.marker; other.marker = tmp; }
			/// The size of the sparse row, i.e. the total number of nonzero elements.
			INMOST_DATA_ENUM_TYPE   Size() const { return static_cast<INMOST_DATA_ENUM_TYPE>(data.size()); }
			INMOST_DATA_ENUM_TYPE & GetIndex(INMOST_DATA_ENUM_TYPE k) {assert(k < data.size()); return (data.begin()+k)->first;}
			INMOST_DATA_REAL_TYPE & GetValue(INMOST_DATA_ENUM_TYPE k) {assert(k < data.size()); return (data.begin()+k)->second;}
			INMOST_DATA_ENUM_TYPE   GetIndex(INMOST_DATA_ENUM_TYPE k) const {assert(k < data.size()); return (data.begin()+k)->first;}
			INMOST_DATA_REAL_TYPE   GetValue(INMOST_DATA_ENUM_TYPE k) const {assert(k < data.size()); return (data.begin()+k)->second;}
			
			iterator                Begin() {return data.begin();}
			iterator                End() {return data.end();}
			const_iterator          Begin() const {return data.begin();}
			const_iterator          End() const {return data.end();}
			reverse_iterator        rBegin() { return data.rbegin(); }
			reverse_iterator        rEnd() { return data.rend(); }
			const_reverse_iterator  rBegin() const { return data.rbegin(); }
			const_reverse_iterator  rEnd() const { return data.rend(); }
			/// Return the scalar product of the current sparse row by a dense Vector.
			INMOST_DATA_REAL_TYPE   RowVec(Vector & x) const; // returns A(row) * x
			void                    MoveRow(Row & new_pos) {data = new_pos.data;} //here move constructor and std::move may be used in future
			/// Set the vector entries by zeroes.
			void                    Zero() {for(iterator it = Begin(); it != End(); ++it) it->second = 0;}
			/// Push specified element into sparse row.
			/// This function should be used only if the index is not repeated in the row.
			void                    Push(INMOST_DATA_ENUM_TYPE ind, INMOST_DATA_REAL_TYPE val) {data.push_back(make_entry(ind,val));}
			/// Resize row to specified size. 
			/// It is intended to be used together with non-const Row::GetIndex and Row::GetValue
			/// that allow for the modification of individual entries.
			/// @param size New size of the row.
			void                    Resize(INMOST_DATA_ENUM_TYPE size) {data.resize(size);}

			void                    Print() 
			{
				for(iterator it = Begin(); it != End(); ++it) std::cout << "(" << it->first << "," << it->second << ") ";
				std::cout << std::endl;
			}
		};


		
		
		/// Class to store the distributed sparse matrix by compressed rows.
		/// The format used to store sparse matrix is analogous to Compressed Row Storage format (CRS).
		/// @see http://netlib.org/linalg/html_templates/node91.html
		class Matrix
		{
		public:
			typedef interval<INMOST_DATA_ENUM_TYPE,Solver::Row> Rows;
			typedef Rows::iterator iterator;
			typedef Rows::const_iterator const_iterator;
		private:
			INMOST_MPI_Comm comm;
			Rows data;
			std::string name;
			bool is_parallel;
		public:
			/// Main constructor of the Matrix class.
			/// @param _name Name of the matrix, empty string by default.
			/// @param start Start of the local data interval.
			/// @param end End of the local data interval.
			/// @param _comm Communicator for parallel data exchanges, MPI_COMM_WORLD by default.
			Matrix(std::string _name = "", INMOST_DATA_ENUM_TYPE start = 0, INMOST_DATA_ENUM_TYPE end = 0, INMOST_MPI_Comm _comm = INMOST_MPI_COMM_WORLD);
			Matrix(const Matrix & other);
			Matrix & operator =(Matrix const & other);
			~Matrix();
			
			/// Return reference to i-th Row of the matrix.
			Row & operator [](INMOST_DATA_ENUM_TYPE i) {return data[i];}
			/// Return reference to i-th Row of the matrix.
			const Row & operator [](INMOST_DATA_ENUM_TYPE i) const {return data[i];}
			/// Return the total number of rows in the matrix.
			INMOST_DATA_ENUM_TYPE  Size() const { return static_cast<INMOST_DATA_ENUM_TYPE>(data.size()); }
			bool                 Empty() const {return data.empty();}
			iterator             Begin() {return data.begin();}
			iterator             End()   {return data.end();}
			const_iterator       Begin() const {return data.begin();}
			const_iterator       End() const   {return data.end();}
			/// Set the start and the end row numbers of the distributed matrix interval.
			void                 SetInterval(INMOST_DATA_ENUM_TYPE   start, INMOST_DATA_ENUM_TYPE   end)       {data.set_interval_beg(start); data.set_interval_end(end);}
			/// Get the start and the end row numbers of the distributed matrix interval.
			void                 GetInterval(INMOST_DATA_ENUM_TYPE & start, INMOST_DATA_ENUM_TYPE & end) const {start = data.get_interval_beg(); end = data.get_interval_end();}
			void                 ShiftInterval(INMOST_DATA_ENUM_TYPE shift) {data.shift_interval(shift);}
			/// Get the first row index of the distributed matrix interval.
			INMOST_DATA_ENUM_TYPE  GetFirstIndex() const {return data.get_interval_beg();}
			/// Get the communicator which the matrix is associated with.
			INMOST_MPI_Comm        GetCommunicator() const {return comm;}
			void                 MoveRows(INMOST_DATA_ENUM_TYPE from, INMOST_DATA_ENUM_TYPE to, INMOST_DATA_ENUM_TYPE size); //for parallel
			void                 Swap(Solver::Matrix & other) 
			{
				data.swap(other.data);
				name.swap(other.name);
				INMOST_MPI_Comm ctmp = comm;
				comm = other.comm;
				other.comm = ctmp;
				bool ptmp = is_parallel;
				is_parallel = other.is_parallel;
				other.is_parallel = ptmp;
			}
			
			/// Matrix-vector product of the form: y = alpha*A*x + beta * y.
			/// @param y Input/output vector.
			void MatVec(INMOST_DATA_REAL_TYPE alpha, Solver::Vector & x, INMOST_DATA_REAL_TYPE beta, Solver::Vector & y) const; //y = alpha*A*x + beta * y
			void MatVecTranspose(INMOST_DATA_REAL_TYPE alpha, Solver::Vector & x, INMOST_DATA_REAL_TYPE beta, Solver::Vector & y) const; //y = alpha*At*x + beta * y

			/// Clear all data of the matrix.
			void Clear() {for(Matrix::iterator it = Begin(); it != End(); ++it) it->Clear(); data.clear();}

			/// Load the matrix from a single data file in MTX format using the specified interval.
			/// If interval is not specified, then it will be automatically constructed,
			/// with the about equal block size (the last block may has larger dimension).
			void				 Load(std::string file, INMOST_DATA_ENUM_TYPE beg = ENUMUNDEF, INMOST_DATA_ENUM_TYPE end = ENUMUNDEF);
			/// Save the distributed matrix to a single data file in MTX format using parallel MPI I/O.
			/// @see http://math.nist.gov/MatrixMarket/formats.html
			void                 Save(std::string file);
			bool &               isParallel() { return is_parallel; }
			
			/// Get the matrix name specified in the main constructor.
			std::string          GetName() {return name;}
			//~ friend class Solver;
		};

		/// This class may be used to sum multiple sparse rows.
		/// \warning
		/// In parallel column indices of the matrix may span wider then 
		/// local row indices, to prevent any problem you are currently
		/// advised to set total size of the matrix as interval of the
		/// RowMerger. In future this may change, see todo 2 below.
		/// \todo
		/// 1. Add iterators over entries.
		/// 2. Implement multiple intervals for distributed computation,
		///    then in parallel the user may specify additional range of indexes
		///    for elements that lay on the borders between each pair of processors.
		class RowMerger
		{
		public:
			static const INMOST_DATA_ENUM_TYPE EOL = ENUMUNDEF-1; ///< End of linked list.
			static const INMOST_DATA_ENUM_TYPE UNDEF = ENUMUNDEF; ///< Value not defined in linked list.
		private:
			bool Sorted; ///< Contents of linked list should be sorted.
			INMOST_DATA_ENUM_TYPE Nonzeros; ///< Number of nonzero in linked list.
			interval< INMOST_DATA_ENUM_TYPE, Row::entry > LinkedList; ///< Storage for linked list.
		public:
			/// Default constructor without size specfied.
			RowMerger();
			/// Constructor with size specified.
			/// @param interval_begin First index in linked list.
			/// @param interval_end Last index in linked list.
			/// @param Sorted Result should be sorted.
			RowMerger(INMOST_DATA_ENUM_TYPE interval_begin, INMOST_DATA_ENUM_TYPE interval_end, bool Sorted = true);
			/// Constructor that gets sizes from matrix
			/// @param A Matrix to get sizes from.
			/// @param Sorted Result should be sorted.
			RowMerger(Matrix & A, bool Sorted = true);
			/// Destructor.
			~RowMerger();
			/// Resize linked list for new interval.
			/// \warning
			/// All contents of linked list will be lost after resize.
			/// This behavior may be changed in future.
			/// @param interval_begin First index in linked list.
			/// @param interval_end Last index in linked list.
			/// @param Sorted Result should be sorted.
			void Resize(INMOST_DATA_ENUM_TYPE interval_begin, INMOST_DATA_ENUM_TYPE interval_end, bool Sorted = true);
			/// Resize linked list for new matrix.
			/// \warning
			/// All contents of linked list will be lost after resize.
			/// This behavior may be changed in future.
			/// @param A Matrix to get sizes from.
			/// @param Sorted Result should be sorted.
			void Resize(Matrix & A, bool Sorted = true);
			/// Clear linked list.
			void Clear();
			/// Add a row with a coefficient into empty linked list.
			/// This routing should be a bit faster then Solver::RowMerger::AddRow
			/// for empty linked list. It may result in an unexpected behavior
			/// for non-empty linked list, asserts will fire in debug mode.
			/// @param coef Coefficient to multiply row values.
			/// @param r A row to be added.
			/// @param PreSortRow Sort values of the row before adding. Will be activated only for sorted linked lists.
			void PushRow(INMOST_DATA_REAL_TYPE coef, Row & r, bool PreSortRow = false);
			/// Add a row with a coefficient into non-empty linked list.
			/// Use Solver::RowMerger::PushRow for empty linked list.
			/// @param coef Coefficient to multiply row values.
			/// @param r A row to be added.
			/// @param PreSortRow Sort values of the row before adding. Will be activated only for sorted linked lists.
			void AddRow(INMOST_DATA_REAL_TYPE coef, Row & r, bool PreSortRow = false);
			/// Multiply all entries of linked list by a coefficient.
			/// @param coef A coefficient for multiplication.
			void Multiply(INMOST_DATA_REAL_TYPE coef);
			/// Place entries from linked list into row.
			/// \warning
			/// All contents of the row will be overwritten.
			/// If you want contents of the row to be added
			/// use AddRow with this row in advance.
			/// @param r A row to be filled.
			void RetriveRow(Row & r);
			//INMOST_DATA_REAL_TYPE ScalarProd(RowMerger & other);
			/// Get current number of nonzeros from linked list.
			INMOST_DATA_ENUM_TYPE Size() {return Nonzeros;}
		};
		
	private:
		static bool is_initialized, is_finalized;
		INMOST_MPI_Comm comm;
		std::string name;
		INMOST_DATA_ENUM_TYPE local_size, global_size;
		INMOST_DATA_ENUM_TYPE last_it;
		INMOST_DATA_REAL_TYPE last_resid;
		OrderInfo info;

		INMOST_DATA_ENUM_TYPE additive_schwartz_overlap;

		INMOST_DATA_ENUM_TYPE maximum_iterations;
		INMOST_DATA_REAL_TYPE absolute_tolerance;
		INMOST_DATA_REAL_TYPE relative_tolerance;
		INMOST_DATA_REAL_TYPE divergence_tolerance;

		INMOST_DATA_REAL_TYPE preconditioner_drop_tolerance;
		INMOST_DATA_REAL_TYPE preconditioner_reuse_tolerance;
		INMOST_DATA_REAL_TYPE preconditioner_ddpq_tolerance;
		INMOST_DATA_ENUM_TYPE preconditioner_reorder_nonzero;
		INMOST_DATA_REAL_TYPE preconditioner_fill_level;
		INMOST_DATA_ENUM_TYPE preconditioner_rescale_iterations;
		INMOST_DATA_ENUM_TYPE preconditioner_condition_estimation;
		INMOST_DATA_ENUM_TYPE preconditioner_adapt_ddpq_tolerance;

		INMOST_DATA_ENUM_TYPE solver_gmres_substeps;

		std::string return_reason;
		
		void * solver_data;
		void * matrix_data;
		void * precond_data;
		
		void * rhs_data;
		void * solution_data;
		
		Type _pack;
		Solver(const Solver & other);// prohibit copy
		Solver & operator =(Solver const & other); //prohibit assignment
	public:
		/// Set the solver parameter of the integer type.
		/// You can find defaults for parameters in the top of the file inmost_solver.h.
		///
		/// Parameters:
		/// - "maximum_iterations" - total number of iterations
		/// - "schwartz_overlap"   - number of overlapping levels for additive schwartz method,
		///                          works for: 
		///                          INNER_ILU2, INNER_MLILUC
		///                          Trilinos_Aztec, Trilinos_Belos, Trilinos_ML, Trilinos_Ifpack
		///                          PETSc
		/// - "gmres_substeps"     - number of gmres steps performed after each bicgstab step,
		///                          works for:
		///                          INNER_ILU2, INNER_MLILUC
		/// - "reorder_nonzeros"   - place sparser rows at the beggining of matrix during reordering,
		///                          works for:
		///                          INNER_MLILUC
		/// - "rescale_iterations" - number of iterations for two-side matrix rescaling,
		///                          works for:
		///                          INNER_ILU2, INNER_MLILUC
		/// - "condition_estimation" - exploit condition estimation of inversed factors to adapt
		///                          drop and reuse tolerances,
		///                          works for:
		///                          INNER_MLILUC
		/// - "adapt_ddpq_tolerance" - adapt ddpq tolerance depending from the complexity 
		//                           of calculation of Schur complement,
		///                          works for:
		///                          INNER_MLILUC
		void SetParameterEnum(std::string name, INMOST_DATA_ENUM_TYPE value);
		/// Set the solver parameter of the real type.
		/// You can find defaults for parameters in the top of the file inmost_solver.h.
		///
		/// Parameters:
		/// - "absolute_tolerance" - iterative method will stop on i-th iteration
		///                          if ||A x(i)-b|| < absolute_tolerance
		/// - "relative_tolerance" - iterative method will stop on i-th iteration
		///                          if ||A x(i)-b||/||A x(0) - b||
		/// - "divergence_tolerance" - iterative method will fail if
		///                          ||A x(i) - b|| > divergence_tolerance
		/// - "drop_tolerance"     - tolerance for dropping values during incomplete factorization,
		///                          works for:
		///                          INNER_ILU2, INNER_MLILUC
		///                          Trilinos_Aztec, Trilinos_Ifpack
		///                          PETSc
		/// - "reuse_tolerance"    - tolerance for reusing values during incomplete factorization,
		///                          these values are used only during calculation of L and U factors
		///                          and/or Schur complement and discarded once factorization is done,
		///                          value should be less then "drop_tolerance",
		///                          typical value is drop_tolerance^2,
		///                          works for:
		///                          INNER_ILU2, INNER_MLILUC
		/// - "ddpq_tolerance"     - by this tolerance most diagonnaly-dominant elements will be selected
		///                          to form the next level of factorization, the closer the tolerance
		///                          is to one the smaller will be the level. Actual rule is:
		///                          A(i,j)/(sum(A(i,:))+sum(A(:,j))-A(i,j)) > ddpq_tolerance *
		///                          A(imax,jmax)/(sum(A(imax,:))+sum(A(:,jmax))-A(imax,jmax))
		///                          where on imax, jmax maximum is reached.
		///                          works for:
		///                          INNER_MLILUC
		/// - "fill_level"         - level of fill for ILU-type preconditioners,
		///                          works for:
		///                          INNER_ILU2 (if LFILL is defined in solver_ilu2.hpp)
		///                          Trilinos, Trilinos_Ifpack
		void SetParameterReal(std::string name, INMOST_DATA_REAL_TYPE value);
		/// Get the used defined name of the Solver.
		std::string          GetName() {return name;}
	
		/// Get the package Type.
		Type GetPackage() const {return _pack;}
		
		/// Return the number of iterations performed by the last solution.
		/// @see Solver::Solve
		INMOST_DATA_ENUM_TYPE Iterations();
		/// Return the final residual achieved by the last solution.
		/// @see Solver::Solve
		INMOST_DATA_REAL_TYPE Residual();
		/// Set the matrix and construct the preconditioner.
		/// @param A Matrix A in linear problem Ax = b
		/// @param OldPreconditioner If this parameter is set to true,
		/// then the previous preconditioner will be used,
		/// otherwise the new preconditioner will be constructed. 
		///
		/// Preconditioner will be constructed on call to this function
		/// - for INNER_*, PETSc and ANI packages
		/// - for Trilinos preconditioner will be constructed each time Solver::Solve is called
		///
		/// Any changes to preconditioner parameters should happen before that point.
		/// If you increase gmres_substep after this point, inner methods most likely will fail
		void SetMatrix(Matrix & A, bool OldPreconditioner = false);
		/// Solver the linear system: A*x = b.
		/// Prior to this call you should call SetMatrix
		///
		/// @param RHS The right-hand side Vector b.
		/// @param SOL The initial guess to the solution on input and the solution Vector x on return.
		///
		/// It is assumed that the coefficient matrix A have been set
		/// and the preconditioner have been already constructed.
		///
		/// @see Solver::SetMatrix
		bool Solve(Vector & RHS, Vector & SOL);
		/// Get the reason of convergence or divergence of the last solution.
		/// @see Solver::Solve
		std::string GetReason();

		/// Main constructor of the solver.
		/// Solver name provided here is used to extract options from database file
		/// for PETSc and Trilinos packages.
		/// @param pack The package Type to be used for solution.
		/// @param _name The user specified name of the current solver.
		/// @param comm Communicator for parallel data exchanges, MPI_COMM_WORLD by default.
		/// @see Solver::Initialize
		/// @see Solver::SetMatrix
		/// @see Solver::Solve
		/// @see Solver::Finalize
		Solver(Type pack, std::string _name = "", INMOST_MPI_Comm comm = INMOST_MPI_COMM_WORLD);
		~Solver();
		/// Initialize the stage of parallel solution.
		/// If MPI is not initialized yet, then it will be initialized.
		///
		/// database file is used to pass parameters to PETSc and Trilinos packages.
		/// if database file for is provided any changes through SetParameterEnum,
		/// SetParameterReal would not be effective for PETSc and Trilinos packages.
		/// Currently this database file provides directions for package-specific
		/// files. In future it is supposed to set up parameters for internal solvers.
		/// @param argc The number of arguments transmitted to the function main.
		/// @param argv The pointer to arguments transmitted to the function main.
		/// @param database Usually the name of the file with the Solver parameters.
		///
		/// The shortest call to this function with the default solver parameters is the following: Initialize(NULL,NULL,"");
		/// @see Solver::Finalize
		/// @see Solver::isInitialized
		///
		/// Example of contents of the database file:
		///
		/// 	PETSc: petsc_options.txt
		/// 	Trilinos_Ifpack: trilinos_ifpack_options.xml
		/// 	Trilinos_ML: trilinos_ml_options.xml
		/// 	Trilinos_Aztec: trilinos_aztec_options.xml
		/// 	Trilinos_Belos: trilinos_belos_options.xml
		static void Initialize(int * argc, char *** argv, const char * database = "");
		/// Finalize the stage of parallel solution.
		/// If MPI was initialized in Solver::Initialize, then it will be finalized.
		/// By this reason, do not use any MPI function after call to this function.
		/// @see Solver::Initialize
		/// @see Solver::isFinalized
		static void Finalize();
		static bool isInitialized() {return is_initialized;}
		static bool isFinalized() {return is_finalized;}

		/// Clear all internal data of the current solver including matrix, preconditioner etc.
		void Clear();
	};
}

#endif // USE_SOLVER

#endif // INMOST_SOLVER_INCLUDED
