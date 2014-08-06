#pragma once
#ifndef __SOLVER_BCGS__
#define __SOLVER_BCGS__

//TODO:
// comply solvers with Method prototype, after TODO in solver_prototypes.hpp is done


#include "inmost_solver.h"

//#if !defined(NDEBUG)
#define REPORT_RESIDUAL
//#endif

namespace INMOST
{
	class BCGSL_solver : public IterativeMethod
	{
		INMOST_DATA_REAL_TYPE rtol, atol, divtol, last_resid;
		INMOST_DATA_ENUM_TYPE iters, maxits, l, last_it;
		INMOST_DATA_REAL_TYPE resid;
		INMOST_DATA_REAL_TYPE * tau, * sigma, * gamma, *theta1, * theta2, * theta3;
		Solver::Vector r0, t, * u, * r;
		Solver::Matrix * Alink;
		Method * prec;
		std::string reason;
		Solver::OrderInfo * info;
		bool init;
	public:
		INMOST_DATA_ENUM_TYPE GetIterations() {return last_it;}
		INMOST_DATA_REAL_TYPE GetResidual() {return last_resid;}
		INMOST_DATA_REAL_TYPE & RealParameter(std::string name)
		{
			if (name[0] == ':')
			{
				if (prec != NULL) return prec->RealParameter(name.substr(1, name.size() - 1));
			}
			if (name == "rtol") return rtol;
			else if (name == "atol") return atol;
			else if (name == "divtol") return divtol;
			else if (prec != NULL) return prec->RealParameter(name);
			throw - 1;
		}
		INMOST_DATA_ENUM_TYPE & EnumParameter(std::string name)
		{
			if (name[0] == ':')
			{
				if (prec != NULL) return prec->EnumParameter(name.substr(1, name.size() - 1));
			}
			if (name == "maxits") return maxits;
			else if (name == "levels") return l;
			else if (prec != NULL) return prec->EnumParameter(name);
			throw - 1;
		}
		BCGSL_solver(Method * prec, Solver::OrderInfo & info)
			:rtol(1e-8), atol(1e-9), divtol(1e+40), maxits(1500),l(2),prec(prec),info(&info)
		{
			Alink = NULL;
			init = false;
		}
		bool Initialize()
		{
			if (isInitialized()) Finalize();
			if (prec != NULL && !prec->isInitialized()) prec->Initialize();
			info->PrepareVector(r0);
			info->PrepareVector(t);
			tau = new INMOST_DATA_REAL_TYPE[l * 5 + l*l];
			sigma = tau + l*l;
			gamma = sigma + l;
			theta1 = gamma + l;
			theta2 = theta1 + l;
			theta3 = theta2 + l;
			u = new Solver::Vector[l * 2 + 2];
			r = u + l + 1;
			for (INMOST_DATA_ENUM_TYPE k = 0; k < l + 1; k++)
			{
				info->PrepareVector(r[k]);
				info->PrepareVector(u[k]);
			}
			init = true;
			return true;
		}
		bool isInitialized() { return init && (prec == NULL || prec->isInitialized()); }
		bool Finalize()
		{
			if (isInitialized())
			{
				if (!prec->isFinalized()) prec->Finalize();
				delete[] u;
				delete[] tau;
				init = false;
			}
			return true;
		}
		bool isFinalized() { return !init && (prec == NULL || prec->isFinalized()); }
		void Copy(const Method * other)
		{
			const BCGSL_solver * b = dynamic_cast<const BCGSL_solver *>(other);
			assert(b != NULL);
			rtol = b->rtol;
			atol = b->atol;
			divtol = b->divtol;
			last_resid = b->last_resid;
			iters = b->iters;
			maxits = b->maxits;
			l = b->l;
			last_it = b->last_it;
			resid = b->resid;
			Alink = b->Alink;
			info = b->info;
			if (init) Finalize();
			if (b->prec != NULL)
			{
				if (prec == NULL) prec = b->prec->Duplicate();
				else prec->Copy(b->prec);
			}
			if (b->init) Initialize();
		}
		BCGSL_solver(const BCGSL_solver & other)
		{
			Copy(&other);
		}
		BCGSL_solver & operator =(BCGSL_solver const & other)
		{
			Copy(&other);
			return *this;
		}
		~BCGSL_solver()
		{
			if (!isFinalized()) Finalize();
			if (prec != NULL) delete prec;
		}
		bool Solve(Solver::Vector & RHS, Solver::Vector & SOL)
		{
			assert(isInitialized());
			INMOST_DATA_ENUM_TYPE vbeg,vend, vlocbeg, vlocend;
			INMOST_DATA_REAL_TYPE rho0 = 1, rho1, alpha = 0, beta, omega = 1;
			INMOST_DATA_REAL_TYPE resid0, resid, temp[2];
			bool is_parallel = info->GetSize() > 1;
			iters = 0;
			info->PrepareVector(SOL);
			info->PrepareVector(RHS);
			if( is_parallel ) info->Update(SOL);
			if( is_parallel ) info->Update(RHS);
			if( prec != NULL ) prec->ReplaceSOL(SOL);
			if( prec != NULL ) prec->ReplaceRHS(RHS);
			info->GetLocalRegion(info->GetRank(),vlocbeg,vlocend);
			info->GetVectorRegion(vbeg,vend);
			std::copy(RHS.Begin(),RHS.End(),r[0].Begin());
			{
				Alink->MatVec(-1,SOL,1,r[0]); //global multiplication, r probably needs an update
				if( is_parallel ) info->Update(r[0]); // r is good
			}
			std::copy(r[0].Begin(),r[0].End(),r0.Begin());
			std::fill(u[0].Begin(),u[0].End(),0);
			{
				resid = 0;
				for(INMOST_DATA_ENUM_TYPE k = vlocbeg; k != vlocend; k++) 
					resid += r[0][k]*r[0][k];
				if( is_parallel ) info->Integrate(&resid,1);
			}
			last_resid = resid = resid0 = sqrt(resid);
			last_it = 0;
#if defined(REPORT_RESIDUAL)
			if( info->GetRank() == 0 ) 
			{
				//std::cout << "iter " << last_it << " residual " << resid << std::endl;
				//std::cout << "iter " << last_it << " resid " << resid << "\r";
				//printf("iter %3d resid %12g | %12g relative %12g | %12g\r", last_it, resid, atol, resid / resid0, rtol);
				printf("iter %3d resid %12g | %g\r", last_it, resid, atol);
				fflush(stdout);
			}
#endif
			long double tt, ts, tp, ttt;
			INMOST_DATA_ENUM_TYPE i = 0;
			while( true )
			{
				ts = tp = 0;
				rho0 = -omega*rho0;
				
				tt = Timer();
				for(INMOST_DATA_ENUM_TYPE j = 0; j < l; j++)
				{
					rho1 = 0.0;
					for(INMOST_DATA_ENUM_TYPE k = vlocbeg; k < vlocend; ++k)
						rho1 += r[j][k]*r0[k];
					if( is_parallel ) info->Integrate(&rho1,1);
					if( fabs(rho0) < 1.0e-54 ) 
					{
						reason = "denominator(1) is zero";
						goto exit;
					}
					beta = alpha * rho1/rho0;
					rho0 = rho1;
					for(INMOST_DATA_ENUM_TYPE i = 0; i < j+1; i++)
						for(INMOST_DATA_ENUM_TYPE k = vbeg; k < vend; ++k)
							u[i][k] = r[i][k] - beta*u[i][k];
					{
						ttt = Timer();
						if (prec != NULL) prec->Solve(u[j], t);
						tp += Timer() - ttt;
						if( is_parallel ) info->Update(t);
						ttt = Timer();
						Alink->MatVec(1.0,t,0,u[j+1]);
						ts += Timer() - ttt;
						if( is_parallel ) info->Update(u[j+1]);
					}
					{
						temp[0] = 0;
						for(INMOST_DATA_ENUM_TYPE k = vlocbeg; k < vlocend; ++k)
							temp[0] += u[j+1][k]*r0[k];
						if( is_parallel ) info->Integrate(temp,1);
						if( fabs(temp[0]) < 1.0e-54 ) 
						{
							reason = "denominator(2) is zero";
							goto exit;
						}
						alpha = rho0 / temp[0];
					}
					for(INMOST_DATA_ENUM_TYPE i = 0; i < j+1; i++)
						for(INMOST_DATA_ENUM_TYPE k = vbeg; k < vend; ++k)
							r[i][k] -= alpha*u[i+1][k];
					{
						ttt = Timer();
						if (prec != NULL) prec->Solve(r[j], t);
						tp += Timer() - ttt;
						if( is_parallel ) info->Update(t);
						ttt = Timer();
						Alink->MatVec(1.0,t,0,r[j+1]);
						ts += Timer() - ttt;
						if( is_parallel ) info->Update(r[j+1]);
					}
					for(INMOST_DATA_ENUM_TYPE k = vbeg; k < vend; ++k)
						SOL[k] += alpha*u[0][k];
				}

				for(INMOST_DATA_ENUM_TYPE j = 1; j < l+1; j++)
				{
					for(INMOST_DATA_ENUM_TYPE i = 1; i < j; i++)
					{
						tau[i-1 + (j-1)*l] = 0;
						for(INMOST_DATA_ENUM_TYPE k = vlocbeg; k < vlocend; ++k)
							tau[i-1 + (j-1)*l] += r[j][k]*r[i][k];
						if(is_parallel) info->Integrate(&tau[i-1 + (j-1)*l],1);
						tau[i-1 + (j-1)*l] /= sigma[i-1];
						for(INMOST_DATA_ENUM_TYPE k = vbeg; k < vend; ++k)
							r[j][k] -= tau[i-1 + (j-1)*l]*r[i][k];
					}
					temp[0] = 0;
					temp[1] = 0;
					for(INMOST_DATA_ENUM_TYPE k = vlocbeg; k < vlocend; ++k)
					{
						temp[0] += r[j][k]*r[j][k];
						temp[1] += r[0][k]*r[j][k];
					}
					if(is_parallel) info->Integrate(temp,2);
					sigma[j-1] = temp[0]+1.0e-35; //REVIEW
					theta2[j-1] = temp[1]/sigma[j-1];
				}
				omega = theta1[l-1] = theta2[l-1];
				for(INMOST_DATA_ENUM_TYPE j = l-1; j > 0; j--)
				{
					temp[0] = 0;
					for(INMOST_DATA_ENUM_TYPE i = j+1; i < l+1; i++)
						temp[0] += tau[j-1 + (i-1)*l] * theta1[i-1];
					theta1[j-1] = theta2[j-1] - temp[0];
				}
				for(INMOST_DATA_ENUM_TYPE j = 1; j < l; j++)
				{
					temp[0] = 0;
					for(INMOST_DATA_ENUM_TYPE i = j+1; i < l; i++)
						temp[0] += tau[j-1 + (i-1)*l] * theta1[i];
					theta3[j-1] = theta1[j] + temp[0];
				}
				for(INMOST_DATA_ENUM_TYPE k = vbeg; k < vend; ++k)
				{
					SOL[k] += theta1[0]*r[0][k];
					r[0][k] -= theta2[l-1]*r[l][k];
					u[0][k] -= theta1[l-1]*u[l][k];
				}
				for(INMOST_DATA_ENUM_TYPE j = 1; j < l; j++)
				{
					for(INMOST_DATA_ENUM_TYPE k = vbeg; k < vend; ++k)
					{
						u[0][k] -= theta1[j-1]*u[j][k];
						SOL[k] += theta3[j-1]*r[j][k];
						r[0][k] -= theta2[j-1]*r[j][k];
					}
				}
				last_it = i+1;
				{
					resid = 0;
					for(INMOST_DATA_ENUM_TYPE k = vlocbeg; k != vlocend; k++) 
						resid += r[0][k]*r[0][k];
					if( is_parallel ) info->Integrate(&resid,1);
					resid = sqrt(resid);
				}
				tt = Timer() - tt;
#if defined(REPORT_RESIDUAL)
				if( info->GetRank() == 0 ) 
				{
					//std::cout << "iter " << last_it << " residual " << resid << " time " << tt << " matvec " << ts*0.5/l << " precond " << tp*0.5/l << std::endl;
					//std::cout << "iter " << last_it << " resid " << resid << "\r";
					//printf("iter %3d resid %12g | %12g relative %12g | %12g\r", last_it, resid, atol, resid / resid0, rtol);
					printf("iter %3d resid %12g | %g\r", last_it, resid, atol);
					fflush(stdout);
				}
#endif
				last_resid = resid;
				if( resid != resid )
				{
					reason = "residual is NAN";
					break;
				}
				if( resid < atol )
				{
					reason = "converged due to absolute tolerance";
					break;
				}
				if( resid < rtol*resid0 )
				{
					reason = "converged due to relative tolerance";
					break;
				}
				if( resid > divtol )
				{
					reason = "diverged due to divergence tolerance";
					break;
				}
				if( i == maxits )
				{
					reason = "reached maximum iteration number";
					break;
				}
				i++;
			}
			if (prec != NULL)
			{
				prec->Solve(SOL, r0);
				std::copy(r0.Begin(), r0.End(), SOL.Begin());
			}
exit:
			//info->RestoreMatrix(A);
			info->RestoreVector(SOL);
			info->RestoreVector(RHS);
			if( last_resid < atol || last_resid < rtol*resid0 ) return true;
			return false;
		}
		bool ReplaceMAT(Solver::Matrix & A) { if (isInitialized()) Finalize(); if (prec != NULL) prec->ReplaceMAT(A);  Alink = &A; return true; }
		bool ReplaceRHS(Solver::Vector & RHS) { return true; }
		bool ReplaceSOL(Solver::Vector & SOL) { return true; }
		Method * Duplicate() { return new BCGSL_solver(*this);}
		std::string GetReason() {return reason;}
	};


	class BCGS_solver : public IterativeMethod
	{
		INMOST_DATA_REAL_TYPE rtol, atol, divtol, last_resid;
		INMOST_DATA_ENUM_TYPE iters, maxits, last_it;
		INMOST_DATA_REAL_TYPE resid;
		Solver::Vector r0, p, y, s, t, z, r, v;
		Solver::Matrix * Alink;
		Method * prec;
		Solver::OrderInfo * info;
		bool init;
		std::string reason;
	public:
		INMOST_DATA_ENUM_TYPE GetIterations() {return last_it;}
		INMOST_DATA_REAL_TYPE GetResidual() {return last_resid;}
		INMOST_DATA_REAL_TYPE & RealParameter(std::string name)
		{
			if (name[0] == ':')
			{
				if (prec != NULL) return prec->RealParameter(name.substr(1, name.size() - 1));
			}
			if (name == "rtol") return rtol;
			else if (name == "atol") return atol;
			else if (name == "divtol") return divtol;
			else if( prec != NULL ) return prec->RealParameter(name);
			throw - 1;
		}
		INMOST_DATA_ENUM_TYPE & EnumParameter(std::string name)
		{
			if (name[0] == ':')
			{
				if (prec != NULL) return prec->EnumParameter(name.substr(1, name.size() - 1));
			}
			if (name == "maxits") return maxits;
			else if (prec != NULL) return prec->EnumParameter(name);
			throw - 1;
		}
		BCGS_solver(Method * prec, Solver::OrderInfo & info)
			:rtol(1e-8), atol(1e-11), divtol(1e+40), iters(0), maxits(1500),prec(prec),info(&info)
		{
			init = false;
		}
		bool Initialize()
		{
			assert(Alink != NULL);
			if (isInitialized()) Finalize();
			if (prec != NULL && !prec->isInitialized()) prec->Initialize();
			info->PrepareVector(r);
			info->PrepareVector(v);
			info->PrepareVector(p);
			info->PrepareVector(y);
			info->PrepareVector(s);
			info->PrepareVector(t);
			info->PrepareVector(z);
			info->PrepareVector(r0);
			init = true;
			return true;
		}
		bool isInitialized() { return init && (prec == NULL || prec->isInitialized()); }
		bool Finalize()
		{
			if (prec != NULL && !prec->isFinalized()) prec->Finalize();
			init = false;
			return true;
		}
		bool isFinalized() { return !init && (prec == NULL || prec->isFinalized()); }
		void Copy(const Method * other)
		{
			const BCGS_solver * b = dynamic_cast<const BCGS_solver *>(other);
			assert(b != NULL);
			info = b->info;
			rtol = b->rtol;
			atol = b->atol;
			divtol = b->divtol;
			maxits = b->maxits;
			last_resid = b->last_resid;
			iters = b->iters;
			last_it = b->last_it;
			resid = b->resid;
			Alink = b->Alink;
			if (b->prec != NULL)
			{
				if (prec == NULL) prec = b->prec->Duplicate();
				else prec->Copy(b->prec);
			}
			if (b->init) Initialize();
		}
		BCGS_solver(const BCGS_solver & other)
		{
			Copy(&other);
		}
		BCGS_solver & operator =(BCGS_solver const & other)
		{
			Copy(&other);
			return *this;
		}
		~BCGS_solver()
		{
			if (!isFinalized()) Finalize();
			if (prec != NULL) delete prec;
		}
		bool Solve(Solver::Vector & RHS, Solver::Vector & SOL)
		{
			assert(isInitialized());
			INMOST_DATA_REAL_TYPE tempa = 0.0, tempb=0.0;
			INMOST_DATA_ENUM_TYPE vbeg,vend, vlocbeg, vlocend;
			INMOST_DATA_INTEGER_TYPE ivbeg,ivend, ivlocbeg, ivlocend;
			INMOST_DATA_REAL_TYPE rho = 1, alpha = 1, beta, omega = 1;
			INMOST_DATA_REAL_TYPE resid0, resid, temp[2];
			bool is_parallel = info->GetSize() > 1;
			info->PrepareVector(SOL);
			info->PrepareVector(RHS);
			if( is_parallel ) info->Update(SOL);
			if( is_parallel ) info->Update(RHS);
			if (prec != NULL)prec->ReplaceSOL(SOL);
			if (prec != NULL)prec->ReplaceRHS(RHS);
			info->GetLocalRegion(info->GetRank(),vlocbeg,vlocend);
			info->GetVectorRegion(vbeg,vend);
			std::copy(RHS.Begin(),RHS.End(),r.Begin());
			{
				Alink->MatVec(-1,SOL,1,r); //global multiplication, r probably needs an update
				info->Update(r); // r is good
			}
			std::copy(r.Begin(),r.End(),r0.Begin());
			std::fill(v.Begin(),v.End(),0);
			std::fill(p.Begin(),p.End(),0);
			{
				resid = 0;
				for(INMOST_DATA_ENUM_TYPE k = vlocbeg; k != vlocend; k++) 
					resid += r[k]*r[k];
				if( is_parallel ) info->Integrate(&resid,1);
			}
			last_resid = resid = resid0 = sqrt(resid);
			last_it = 0;
			ivbeg = vbeg;
			ivend = vend;
			ivlocbeg = vlocbeg;
			ivlocend = vlocend;
#if defined(REPORT_RESIDUAL)
			if( info->GetRank() == 0 ) 
			{
				//std::cout << "iter " << last_it << " residual " << resid << std::endl;
				//std::cout << "iter " << last_it << " resid " << resid << "\r";
				//printf("iter %3d resid %12g | %12g relative %12g | %12g\r",last_it,resid,atol,resid/resid0,rtol);
				printf("iter %3d resid %12g | %g\r", last_it, resid, atol);
				fflush(stdout);
			}
#endif
#pragma omp parallel
			{
				long double tt, ts, tp, ttt;
				INMOST_DATA_ENUM_TYPE i = 0;
				while(true)
				{
					ts = tp = 0;
					tt = Timer();
					{
						if( fabs(rho) < 1.0e-31 )
						{
							reason = "denominator(1) is zero";
							break;
						}
						if( fabs(omega) < 1.0e-31 )
						{
							reason = "denominator(2) is zero";
							break;
						}
						beta = 1.0 /rho * alpha / omega;
						rho = 0;
#pragma omp for reduction(+:rho)
						for(INMOST_DATA_INTEGER_TYPE k = ivlocbeg; k < ivlocend; k++) 
							rho += r0[k]*r[k];
						if( is_parallel ) 
						{
#pragma omp single
							info->Integrate(&rho,1);
						}
						beta *= rho;
					}
					{
#pragma omp for
						for(INMOST_DATA_INTEGER_TYPE k = ivbeg; k < ivend; ++k) 
							p[k] = r[k] + beta*(p[k] - omega*v[k]); //global indexes r, p, v
					}
					{
						ttt = Timer();
						if (prec != NULL)
						{
#pragma omp single
							prec->Solve(p, y);
						}
						tp += Timer() - ttt;
						if( is_parallel ) 
						{
#pragma omp single
							info->Update(y);
						}
						ttt = Timer();
						Alink->MatVec(1,y,0,v); // global multiplication, y should be updated, v probably needs an update
						ts += Timer() - ttt;
						if( is_parallel ) 
						{
#pragma omp single
							info->Update(v);
						}
					}
					{
						alpha = 0;
#pragma omp for reduction(+:alpha)
						for(INMOST_DATA_INTEGER_TYPE k = ivlocbeg; k < ivlocend; k++)  
							alpha += r0[k]*v[k];
						if( is_parallel ) 
						{
#pragma omp single
							info->Integrate(&alpha,1);
						}
						if( fabs(alpha) < 1.0e-31 )
						{
							reason = "denominator(3) is zero";
							break;
						}

						alpha = rho / alpha; //local indexes, r0, v
					}
					{
#pragma omp for
						for(INMOST_DATA_INTEGER_TYPE k = ivbeg; k < ivend; ++k) 
							s[k] = r[k] - alpha * v[k]; //global indexes r, v
					}
				
					{
						ttt = Timer();
						if (prec != NULL)
						{
#pragma omp single
							prec->Solve(s, z);
						}
						tp += Timer() - ttt;
						if( is_parallel ) 
						{
#pragma omp single
							info->Update(z);
						}
						ttt = Timer();
						Alink->MatVec(1.0,z,0,t); // global multiplication, z should be updated, t probably needs an update
						ts += Timer() - ttt;
						if( is_parallel ) 
						{
#pragma omp single
							info->Update(t);
						}
					}
					{
						
						temp[0] = temp[1] = 0;
#pragma omp for reduction(+:tempa) reduction(+:tempb)
						for(INMOST_DATA_INTEGER_TYPE k = ivlocbeg; k < ivlocend; k++)
						{
							tempa += t[k]*s[k];
							tempb += t[k]*t[k];
						}
						temp[0] = tempa;
						temp[1] = tempb;
						if( is_parallel ) 
						{
#pragma omp single
							info->Integrate(temp,2);
						}
						/*
						if (fabs(temp[0]) < 1.0e-35)
						{
							if (fabs(temp[1]) > 1.0e-35)
								break; //breakdown
							else omega = 0.0;
						}
						else 
						*/
						omega = temp[0] / (temp[1]+1e-35); //local indexes t, s
					}
					{
#pragma omp for
						for(INMOST_DATA_INTEGER_TYPE k = ivbeg; k < ivend; ++k) 
							SOL[k] += alpha * y[k] + omega * z[k]; // global indexes SOL, y, z
					}
					{
#pragma omp for
						for(INMOST_DATA_INTEGER_TYPE k = ivbeg; k < ivend; ++k) 
							r[k] = s[k] - omega * t[k]; // global indexes r, s, t
					}
					last_it = i+1;
					{
						resid = 0;
#pragma omp for reduction(+:resid)
						for(INMOST_DATA_INTEGER_TYPE k = ivlocbeg; k < ivlocend; k++) 
							resid += r[k]*r[k];
						if( is_parallel ) 
						{
#pragma omp single
							info->Integrate(&resid,1);
						}
						resid = sqrt(resid);
					}
					tt = Timer() - tt;
#if defined(REPORT_RESIDUAL)
					if( info->GetRank() == 0 ) 
					{
						//std::cout << "iter " << last_it << " residual " << resid << " time " << tt << " matvec " << ts*0.5 << " precond " << tp*0.5 << std::endl;
						//std::cout << "iter " << last_it << " resid " << resid << "\r";
						//printf("iter %3d resid %12g | %12g relative %12g | %12g\r", last_it, resid, atol, resid / resid0, rtol);
#pragma omp single
						{
							printf("iter %3d resid %12g | %g\r", last_it, resid, atol);
							fflush(stdout);
						}
					}
#endif
					last_resid = resid;
					if( resid != resid )
					{
						reason = "residual is NAN";
						break;
					}
					if( resid > divtol )
					{
						reason = "diverged due to divergence tolerance";
						break;
					}
					if( resid < atol )
					{
						reason = "converged due to absolute tolerance";
						break;
					}
					if( resid < rtol*resid0 )
					{
						reason = "converged due to relative tolerance";
						break;
					}
					if( i == maxits )
					{
						reason = "reached maximum iteration number";
						break;
					}
					i++;
				}
			}
			//info->RestoreMatrix(A);
			info->RestoreVector(SOL);
			info->RestoreVector(RHS);
			if( last_resid < atol || last_resid < rtol*resid0 ) return true;
			return false;
		}
		bool ReplaceMAT(Solver::Matrix & A) { if (isInitialized()) Finalize();  if (prec != NULL) prec->ReplaceMAT(A);  Alink = &A; return true; }
		bool ReplaceRHS(Solver::Vector & RHS) { return true; }
		bool ReplaceSOL(Solver::Vector & SOL) { return true; }
		Method * Duplicate() { return new BCGS_solver(*this);}
		std::string GetReason() {return reason;}
	};
}


#endif