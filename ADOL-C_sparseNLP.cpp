//#define SPARSE_HESS
//#define RETAPE


#include <cassert>
#include "ADOL-C_sparseNLP.hpp"
using namespace Ipopt;

/* Constructor. */
MyADOLC_sparseNLP::MyADOLC_sparseNLP() {

	x_lam				= NULL;
	cind_g 				= NULL;
	rind_g				= NULL;
	jacval 				= NULL;

	rind_L				= NULL;
	cind_L				= NULL;
	rind_L_total		= NULL;
	cind_L_total		= NULL;
	hessval				= NULL;
}

MyADOLC_sparseNLP::~MyADOLC_sparseNLP()
{

}

bool  MyADOLC_sparseNLP::ad_eval_obj(Index n, const adouble *x, adouble& obj_value) {
  // return the value of the objective function
	cout<<"ad_eval_obj\n";

	adouble *y0		= new adouble [n_states];
	adouble *yf		= new adouble [n_states];
	adouble **y 	= new adouble *[n_nodes];
	adouble **u;
	adouble *param	= new adouble [n_param];

	adouble tf, t0;

	if (disc_method == Hermite_Simpson) {
		u 	= new adouble *[2*n_nodes - 1];
		for (Index i = 0; i < 2*n_nodes - 1; i += 1) {
			u[i]	= new adouble [n_controls];
		}
	}
	else {
		u 	= new adouble *[n_nodes];
		for (Index i = 0; i < n_nodes; i += 1) {
			u[i]	= new adouble [n_controls];
		}
	}

	for (Index i = 0; i < n_nodes; i += 1) {
		y[i]	= new adouble [n_states];
	}

	adouble* x_sf = new adouble[n];
	for (Index i = 0; i < n; i++)
		x_sf[i] = this->NLP_x_sf[i];

	NLP_x_2_OCP_var(x, x_sf, y, u, param, t0, tf);

	for (Index i = 0; i < n_states; i += 1)
	{
		y0[i] 		= y[0][i];
		yf[i]		= y[n_nodes - 1][i];
	}

	obj_value = ad_e_cost (y0, yf, param, t0, tf, 1);

	for (Index i = 0; i < n_nodes; i += 1) {
		delete[] y[i];
	}

	if (disc_method == Hermite_Simpson) {
		for (Index i = 0; i < 2*n_nodes - 1; i += 1) {
			delete[] u[i];
		}
	}
	else {
		for (Index i = 0; i < n_nodes; i += 1) {
			delete[] u[i];
		}
	}

	delete[] y;
	delete[] u;
	delete[] y0;
	delete[] yf;
	delete[] param;
	delete[] x_sf;

	cout<<"end ad_eval_obj\n";
	return true;
}

bool  MyADOLC_sparseNLP::eval_obj(Index n, const double *x, double& obj_value) {
  // return the value of the objective function

	NLP_x_2_OCP_var(x,NLP_x_sf,y,u,param,t0,tf);

	for (Index i = 0; i < n_states; i += 1)
	{
		y0[i] 		= y[0][i];
		yf[i]		= y[n_nodes - 1][i];
	}

	obj_value = d_e_cost (y0, yf, param, t0, tf, 1);

	return true;
}

bool  MyADOLC_sparseNLP::ad_eval_constraints(Index n, const adouble *x, Index m, adouble* g) {
	//cout<<"ad_eval_constraints\n";

	adouble *y_start	= new adouble [n_states];
	adouble *y_end		= new adouble [n_states];
	adouble **y 		= new adouble *[n_nodes];
	adouble **f		 	= new adouble *[n_nodes];
	adouble **u;
	adouble **path		= new adouble *[n_nodes];
	adouble *param		= new adouble [n_param];
	adouble *e			= new adouble [n_events];
	adouble **defects	= new adouble *[n_nodes-1];

	adouble *y_m		= new adouble [n_states];
	adouble *f_m		= new adouble [n_states];
	adouble *path_m		= new adouble [n_path];
	adouble *t_m		= new adouble  [n_nodes - 1];

	adouble *delta	 	= new adouble [n_nodes - 1];
	adouble *t	 		= new adouble [n_nodes];

	adouble tf, t0;

	adouble *g_sf		= new adouble [m];

	if (disc_method == Hermite_Simpson) {
		u 	= new adouble *[2*n_nodes - 1];
		for (Index i = 0; i < 2*n_nodes - 1; i += 1) {
			u[i]	= new adouble [n_controls];
		}
	}
	else {
		u 	= new adouble *[n_nodes];
		for (Index i = 0; i < n_nodes; i += 1) {
			u[i]	= new adouble [n_controls];
		}
	}

	for (Index i = 0; i < m; i++)
		g_sf[i]		= this->NLP_g_sf[i];

	for (Index i = 0; i < n_nodes; i += 1) {
		y[i]		= new adouble [n_states];
		path[i]		= new adouble [n_path];
		f[i] 		= new adouble [n_states];
	}

	for (Index i = 0; i < n_nodes - 1; i++) {
		defects[i]	= new adouble [n_states];
	}

	adouble* x_sf = new adouble[n];
	for (Index i = 0; i < n; i++)
		x_sf[i] = this->NLP_x_sf[i];

	NLP_x_2_OCP_var(x,x_sf,y,u,param,t0,tf);

	t[0]				= t0;
	for (Index i = 0; i < n_nodes - 1; i++) {
		delta[i]	= (tf-t0)*node_str(i+1);
		t[i+1]		= t[i] + delta[i];
		t_m[i]		= (t[i] + t[i+1])/2;
	}

	for (Index i = 0; i < n_states; i += 1) {
			y_start[i] 		= y[0][i];
			y_end[i]		= y[n_nodes - 1][i];
	}
	ad_events(e, y_start, y_end, param, t0, tf, 1);

	if (disc_method == Hermite_Simpson) {
		ad_derv(f[0], path[0], y[0], u[0], param, t[0], 1);
		for (Index i = 0; i < n_nodes - 1; i += 1)	{
			ad_derv(f[i+1], path[i+1], y[i+1], u[2*(i+1)], param, t[i+1], 1);
			for (Index j = 0; j < n_states; j += 1){
				y_m[j] 	= (y[i][j]+y[i+1][j])/2 + delta[i]/8*(f[i][j]-f[i+1][j]);
			}
			ad_derv(f_m, path_m, y_m, u[2*i+1], param, t_m[i], 1);
			for (Index j = 0; j < n_states; j++)
				defects[i][j]	= (y[i+1][j] - y[i][j] - delta[i]/6*(f[i][j]+4*f_m[j]+f[i+1][j]));
		}
	}
	else if (disc_method == trapezoidal) {
		for (Index i = 0; i < n_nodes; i += 1)	{
			ad_derv(f[i], path[i], y[i], u[i], param, t[i], 1);
		}
		for (Index i = 0; i < n_nodes - 1; i++)
			for (Index j = 0; j < n_states; j++)
				defects[i][j] 	= y[i+1][j] - y[i][j] - delta[i]/2.0*(f[i][j] + f[i+1][j]);
	}
	OCP_var_2_NLP_g(path, defects, e, g, g_sf);

	for (Index i = 0; i < m; i++)
		g[i]	= g[i] + 1;

	if (disc_method == Hermite_Simpson) {
		for (Index i = 0; i < 2*n_nodes - 1; i += 1) {
			delete[] u[i];
		}
	}
	else {
		for (Index i = 0; i < n_nodes; i += 1) {
			delete[] u[i];
		}
	}

	for (Index i = 0; i < n_nodes; i += 1)
	{
		delete[] y[i];
		delete[] path[i];
		delete[] f[i];
		if (i < n_nodes - 1) {
			delete[] defects[i];
		}
	}

	delete[] y_m;
	delete[] f_m;
	delete[] path_m;

	delete[] y;
	delete[] path;
	delete[] defects;

	delete[] u;
	delete[] f;
   	delete[] y_start;
   	delete[] y_end;
   	delete[] param;
   	delete[] e;
   	delete[] delta;
  	delete[] t;
  	delete[] t_m;
  	delete[] x_sf;
  	delete[] g_sf;

  	//cout<<"end ad_eval_constraints\n";
	return true;
}

bool  MyADOLC_sparseNLP::eval_constraints(Index n, const double *x, Index m, double* g) {

	double* y_m			= new double [n_states];
	double* f_m			= new double [n_states];
	double* path_m		= new double [n_path];
	double*t_m			= new double  [n_nodes - 1];

	NLP_x_2_OCP_var(x,NLP_x_sf,y,u,param,t0,tf);

	for (Index i = 0; i < n_states; i += 1)
	{
		y0[i] 		= y[0][i];
		yf[i]		= y[n_nodes - 1][i];
	}

	d_events(e, y0, yf, param, t0, tf, 1);

	t[0]				= t0;
	for (Index i = 0; i < n_nodes - 1; i++) {
		delta[i]	= (tf-t0)*node_str(i+1);
		t[i+1]		= t[i] + delta[i];
		t_m[i]		= (t[i] + t[i+1])/2;
	}

	if (disc_method == Hermite_Simpson) {
		for (Index i = 0; i < n_nodes; i += 1)	{
			d_derv(f[i], path[i], y[i], u[2*i], param, t[i], 1);
		}
		for (Index i = 0; i < n_nodes - 1; i += 1)	{
			for (Index j = 0; j < n_states; j += 1){
				y_m[j] 	= (y[i][j]+y[i+1][j])/2 + delta[i]/8*(f[i][j]-f[i+1][j]);
			}
			d_derv(f_m, path_m, y_m, u[2*i+1], param, t_m[i], 1);
			for (Index j = 0; j < n_states; j++)
				defects[i][j]	= (y[i+1][j] - y[i][j] - delta[i]/6*(f[i][j]+4*f_m[j]+f[i+1][j]));
		}
	}
	else if (disc_method == trapezoidal) {
		for (Index i = 0; i < n_nodes; i += 1)	{
			d_derv(f[i], path[i], y[i], u[i], param, t[i], 1);
		}
		for (Index i = 0; i < n_nodes - 1; i++)
			for (Index j = 0; j < n_states; j++)
				defects[i][j] 	= y[i+1][j] - y[i][j] - delta[i]/2.0*(f[i][j] + f[i+1][j]);
	}

	OCP_var_2_NLP_g(path, defects, e, g, NLP_g_sf);

	for (Index i = 0; i < m; i++)
		g[i]	= g[i] + 1;

	delete[] y_m;
	delete[] f_m;
	delete[] path_m;
  	delete[] t_m;

	return true;
}

bool MyADOLC_sparseNLP::get_nlp_info(Index& n, Index& m, Index& nnz_jac_g,
                         Index& nnz_h_lag, IndexStyleEnum& index_style)
{
	n = NLP_n;
	m = NLP_m;
	cout<<"n = "<<n<<"\n";
	cout<<"m = "<<m<<"\n";
	cout<<"generate tapes\n";
	generate_tapes(n, m, nnz_jac_g, nnz_h_lag);
	cout<<"end ini\n";
  // use the C style indexing (0-based)
	index_style = C_STYLE;

	return true;
}

bool MyADOLC_sparseNLP::get_bounds_info(Index n, Number* x_l, Number* x_u,
										Index m, Number* g_l, Number* g_u)
{


	for (Index i = 0; i < NLP_n; i += 1) {
		x_l[i] = NLP_x_lb[i];
		x_u[i] = NLP_x_ub[i];
	}
	for (Index i = 0; i < NLP_m; i += 1) {
		g_l[i] = NLP_g_lb[i];
		g_u[i] = NLP_g_ub[i];
	}

	return true;
}

bool MyADOLC_sparseNLP::get_starting_point(Index n, bool init_x, Number* x,
                               bool init_z, Number* z_L, Number* z_U,
                               Index m, bool init_lambda,
                               Number* lambda)
{
	assert(init_x == true);
	assert(init_z == false);
	assert(init_lambda == false);
	cout<<"get starting point\n";

	for (Index i = 0; i < n; i += 1) {
		x[i]	= NLP_x_guess[i];
	}
	cout<<"end of getting starting point\n";
	return true;
}

void 	MyADOLC_sparseNLP::setNLP_structure(Index n, Index m, SMatrix<uint> structure, APPROX method) {
	NLP_n 				= n;
	NLP_m 				= m;

	n_phases			= structure(1);
	n_nodes 			= structure(2);
	n_states 			= structure(3);
	n_controls 			= structure(4);
	n_param 			= structure(5);
	n_events			= structure(6);
	n_path				= structure(7);
	n_linkages			= structure(8);
	disc_method			= method;

	y0			= new double [n_states];
	yf			= new double [n_states];
	y 			= new double *[n_nodes];
	param		= new double [n_param];

	f		 	= new double *[n_nodes];
	path		= new double *[n_nodes];
	defects		= new double *[n_nodes-1];
	e			= new double [n_events];
	t	 		= new double [n_nodes];
	delta	 	= new double [n_nodes - 1];

	for (Index i = 0; i < n_nodes; i += 1) {
		y[i]	= new double [n_states];
		f[i]	= new double [n_states];
		path[i] = new double [n_path];
		if (i < n_nodes - 1)
			defects[i] = new double [n_states];
	}

	if ( disc_method == Hermite_Simpson) {
		u 			= new double *[2*n_nodes - 1];
		for (Index i = 0; i < 2*n_nodes - 1; i += 1) {
			u[i]	= new double [n_controls];
		}
	}
	else {
		u			= new double *[n_nodes];
		for (Index i = 0; i < n_nodes; i += 1) {
			u[i]	= new double [n_controls];
		}
	}
}

void 	MyADOLC_sparseNLP::setBounds (double* x_lb, double* x_ub, double* g_lb, double* g_ub){
	NLP_x_lb = x_lb;
	NLP_x_ub = x_ub;
	NLP_g_lb = g_lb;
	NLP_g_ub = g_ub;
}

void 	MyADOLC_sparseNLP::setSF (double* x_sf, double* g_sf){
	NLP_x_sf = x_sf;
	NLP_g_sf = g_sf;
}

bool MyADOLC_sparseNLP::eval_f(Index n, const Number* x, bool new_x, Number& obj_value)
{
	eval_obj(n,x,obj_value);
	return true;
}

bool MyADOLC_sparseNLP::eval_grad_f(Index n, const Number* x, bool new_x, Number* grad_f)
{
	gradient(tag_f,n,x,grad_f);

	return true;
}

bool MyADOLC_sparseNLP::eval_g(Index n, const Number* x, bool new_x, Index m, Number* g)
{

	eval_constraints(n,x,m,g);
	return true;
}

bool MyADOLC_sparseNLP::eval_jac_g(Index n, const Number* x, bool new_x,
                       Index m, Index nele_jac, Index* iRow, Index *jCol,
                       Number* values)
{

	if (values == NULL) {
	// return the structure of the jacobian
		for(Index idx=0; idx<nnz_jac; idx++) {
			iRow[idx] = rind_g[idx];
			jCol[idx] = cind_g[idx];
		}
	}
	else {

	#ifdef RETAPE
/*		cout<<"retape\n";
		// return the values of the jacobian of the constraints
		adouble *xa   = new adouble[n];
		adouble *ga    = new adouble[m];
		double dummy;

		trace_on(tag_g);

	    for(Index idx=0;idx<n;idx++)
	    	xa[idx] <<= x[idx];

	    ad_eval_constraints(n,xa,m,ga);

	    for(Index idx=0;idx<m;idx++)
			ga[idx] >>= dummy;
	    trace_off();

	    delete[] xa;
	    delete[] ga;
*/
		free(rind_g);
		free(cind_g);
		free(jacval);

		rind_g = NULL;
		cind_g = NULL;
		jacval = NULL;

		sparse_jac(tag_g, m, n, 0, x, &nnz_jac, &rind_g, &cind_g, &jacval, options_g);
		for(Index idx=0; idx<nnz_jac; idx++) {
			values[idx] = jacval[idx];
		}
	}
#else

		sparse_jac(tag_g, m, n, 1, x, &nnz_jac, &rind_g, &cind_g, &jacval, options_g);
		for(Index idx=0; idx<nnz_jac; idx++)
			values[idx] = jacval[idx];
	}
#endif

	return true;
}

bool MyADOLC_sparseNLP::eval_h(Index n, const Number* x, bool new_x,
                   Number obj_factor, Index m, const Number* lambda,
                   bool new_lambda, Index nele_hess, Index* iRow,
                   Index* jCol, Number* values)
{

#ifdef SPARSE_HESS

	if (values == NULL) {
	    // return the structure. This is a symmetric matrix, fill the lower left
	    // triangle only.

		for(Index idx=0; idx<nnz_L; idx++) {
			iRow[idx] = rind_L[idx];
			jCol[idx] = cind_L[idx];
		}
	}
	else {
	    // return the values. This is a symmetric matrix, fill the lower left
	    // triangle only

		for(Index idx = 0; idx<n ; idx++)
			x_lam[idx] = x[idx];
		for(Index idx = 0; idx<m ; idx++)
	    	x_lam[n+idx] = lambda[idx];
		x_lam[n+m] = obj_factor;

		sparse_hess(tag_L, n+m+1, 1, x_lam, &nnz_L_total, &rind_L_total, &cind_L_total, &hessval, options_L);

		Index idx = 0;
		for(Index idx_total = 0; idx_total <nnz_L_total; idx_total++) {
			if((rind_L_total[idx_total] < (unsigned int) n) && (cind_L_total[idx_total] < (unsigned int) n)) {
				values[idx] = hessval[idx_total];
				idx++;
			}
		}
	}
	return true;
#endif
	return false;
}

void MyADOLC_sparseNLP::finalize_solution(SolverReturn status,
                              Index n, const Number* x, const Number* z_L, const Number* z_U,
                              Index m, const Number* g, const Number* lambda,
                              Number obj_value,
			      const IpoptData* ip_data,
			      IpoptCalculatedQuantities* ip_cq)
{

	NLP_x_opt.resize(n,1);
	for (Index i = 0; i < n; i++) {
		NLP_x_opt(i+1) 	= x[i]*NLP_x_sf[i];
	}
	NLP_lam_opt.resize(m,1);
	for (Index i = 0; i < m; i++) {
		NLP_lam_opt(i+1) 	= lambda[i]*NLP_g_sf[i];
	}
// memory deallocation of ADOL-C variables

	for (Index i = 0; i < n_nodes; i += 1) {
		delete[] y[i];
		delete[] path[i];
		delete[] f[i];
		if (i < n_nodes - 1)
			delete[] defects[i];
	}
	if (disc_method == Hermite_Simpson){
		for (Index i = 0; i <2*n_nodes-1; i += 1)
			delete[] u[i];
	}
	else {
		for (Index i = 0; i <n_nodes; i += 1)
			delete[] u[i];
	}

	delete[] y0;
	delete[] yf;
	delete[] y;
	delete[] u;
	delete[] param;

	delete[] path;
	delete[] defects;
	delete[] f;
   	delete[] e;
  	delete[] t;
  	delete[] delta;
	delete[] x_lam;

	delete[] NLP_x_lb;
	delete[] NLP_x_ub;
	delete[] NLP_x_sf;
	delete[] NLP_x_guess;
	delete[] NLP_g_lb;
	delete[] NLP_g_ub;
	delete[] NLP_g_sf;

	free(rind_g);
	free(cind_g);
	free(jacval);

#ifdef SPARSE_HESS
	delete[] (rind_L);
	delete[] (cind_L);
	free(cind_L_total);
	free(rind_L_total);
	free(hessval);
#endif
}

void MyADOLC_sparseNLP::generate_tapes(Index n, Index m, Index& nnz_jac_g, Index& nnz_h_lag)
{
	Number *xp    = new double[n];
	Number *lamp  = new double[m];
	Number *zl    = new double[m];
	Number *zu    = new double[m];

	adouble *xa   = new adouble[n];
	adouble *g    = new adouble[m];
	adouble *lam  = new adouble[m];
	adouble sig;
	adouble obj_value;

	double dummy;

	x_lam   = new double[n+m+1];

	get_starting_point(n, 1, xp, 0, zl, zu, m, 0, lamp);

	trace_on(tag_f);
    
    for(Index idx=0;idx<n;idx++)
      xa[idx] <<= xp[idx];

    ad_eval_obj(n,xa,obj_value);

    obj_value >>= dummy;

    trace_off();

    trace_on(tag_g);

    for(Index idx=0;idx<n;idx++)
    	xa[idx] <<= xp[idx];

    ad_eval_constraints(n,xa,m,g);


    for(Index idx=0;idx<m;idx++)
		g[idx] >>= dummy;
    trace_off();

#ifdef SPARSE_HESS

    trace_on(tag_L);
    
    for(Index idx=0;idx<n;idx++)
    	xa[idx] <<= xp[idx];
    for(Index idx=0;idx<m;idx++)
    	lam[idx] <<= 1.0;
    sig <<= 1.0;

    ad_eval_obj(n,xa,obj_value);

    obj_value = obj_value*sig;
    ad_eval_constraints(n,xa,m,g);
 
    for(Index idx=0;idx<m;idx++)
		obj_value = obj_value + g[idx]*lam[idx];

    obj_value >>= dummy;

    trace_off();
#endif

	rind_g = NULL;
	cind_g = NULL;

	options_g[0] = 0;          /* sparsity pattern by index domains (default) */
	options_g[1] = 0;          /*                         safe mode (default) */
	options_g[2] = -1;         /*                     &jacval is not computed */
	options_g[3] = 0;          /*                column compression (default) */


	this->jacval=NULL;


	sparse_jac(tag_g, m, n, 0, xp, &nnz_jac, &rind_g, &cind_g, &jacval, options_g);

	options_g[2] = 0;
	nnz_jac_g = nnz_jac;

/*
	sparse_jac(tag_g, m, n, 1, xp, &nnz_jac, &rind_g, &cind_g, &jacval, options_g);


	Number* NLP_constraint_sf = new Number[m];
	for (uint i=0; i<m; i++) {
		NLP_constraint_sf[i] = 0.0;
	}


	for (uint i=0;i<nnz_jac;i++) {
		NLP_constraint_sf[rind_g[i]] += jacval[i]*jacval[i];
	}


	for(uint i = 0; i < m; i++) {
		printf("constraint_sf = %e\n",NLP_constraint_sf[i]);
	}
*/
	double *grad_f = new double[n];
	gradient(tag_f,n,xp,grad_f);

	double enorm_grad_f = 0;
	for (Index i = 0; i < n ; i++)
		enorm_grad_f = enorm_grad_f + grad_f[i]*grad_f[i];
	enorm_grad_f = sqrt(enorm_grad_f);
	printf("norm_grad_f = %e\n",enorm_grad_f);
/*
	if(enorm_grad_f != 0 )
		NLP_obj_sf = enorm_grad_f;
*/
/*
	trace_on(tag_f);

    for(Index idx=0;idx<n;idx++)
      xa[idx] <<= xp[idx];

    ad_eval_obj(n,xa,obj_value);

    obj_value >>= dummy;

    trace_off();
*/
/*

	cout<<"eval_grad_f\n";
	for(Index i = 0; i < n; i++) {
		printf("grad_f[%d] = %f\n",i,grad_f[i]);
	}
	for (int i = 0; i < nnz_jac; i += 1)
	{
		printf("sparse_j[%d] = %f\n",i,jacval[i]);
	}
*/
	delete[] grad_f;

#ifdef SPARSE_HESS
	hessval			= NULL;
	cind_L_total	= NULL;
	rind_L_total	= NULL;
	options_L[0]= 0;
	options_L[1]= 0;
	sparse_hess(tag_L, n+m+1, 0, x_lam, &nnz_L_total, &rind_L_total, &cind_L_total, &hessval, options_L);
	nnz_L = 0;
	for(Index idx_total = 0; idx_total <nnz_L_total ; idx_total++) {
		if((rind_L_total[idx_total] < (unsigned int) n) && (cind_L_total[idx_total] < (unsigned int) n)) {
			nnz_L++;
		}
	}
	nnz_h_lag = nnz_L;
	cout<<"nnz_h_lag = "<<nnz_h_lag<<"\n";
	rind_L 		= new unsigned int [nnz_L];
	cind_L 		= new unsigned int [nnz_L];
	Index idx 	= 0;
	for(Index idx_total = 0; idx_total <nnz_L_total ; idx_total++) {
		if((rind_L_total[idx_total] < (unsigned int) n) && (cind_L_total[idx_total] < (unsigned int) n)) {
			rind_L[idx]		= rind_L_total[idx_total];
			cind_L[idx]		= cind_L_total[idx_total];
			idx++;
		}
	}
#endif

	delete[] lam;
	delete[] g;
	delete[] xa;
	delete[] zu;
	delete[] zl;
	delete[] lamp;
	delete[] xp;

}
