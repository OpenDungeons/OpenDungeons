
#include "HermiteCatmullSpline.h"
#include <iostream>


using std::cerr ; using std::endl;

HermiteCatmullSpline::HermiteCatmullSpline(int nn_in):nn(nn_in)   {
    mm = new double[nn];
    pp = new double[nn];
    tt = new double[nn];
    computeTangents();
    }



HermiteCatmullSpline::HermiteCatmullSpline(int nn_in , double *pp_in,double *tt_in):nn(nn_in), pp(pp_in), tt(tt_in)   {

    
    mm = new double[nn];
    computeTangents();
  

    }

HermiteCatmullSpline::~HermiteCatmullSpline(){

    delete[] mm;
    delete[] pp;
    delete[] tt; 
  


    }


double HermiteCatmullSpline::h00(double xx ){

    return ( 1 + 2*xx) + ( 1 - xx)*(1 - xx);



    }


double HermiteCatmullSpline::h10(double xx ){

    return xx*( 1 - xx)*(1 - xx);



    }


double HermiteCatmullSpline::h01(double xx ){

    return xx*xx*(3-2*xx);



    }


double HermiteCatmullSpline::h11(double xx ){

    return xx*xx*(xx-1);



    }


double HermiteCatmullSpline::evaluate_aux( double tk, double tk_pp1 , double mk, double mk_pp1 , double pk, double pk_pp1, double xx ){

    double tt = (xx - tk)/(tk_pp1 - tk);
    return h00(tt)*pk+ h10(tt)*(tk_pp1 - tk )*mk + h10(tt)*pk_pp1 + h01(tt)*(tk_pp1 - tk )*mk_pp1; 



    }


double HermiteCatmullSpline::evaluate(double xx){
    
    
    
    int ii = interval(xx);

    
    if(ii != -1)
	return evaluate_aux(tt[ii], tt[ii+1], mm[ii], mm[ii+1], pp[ii], pp[ii+1], xx  )/2;
    else
	return 0;
    }



double HermiteCatmullSpline::computeTangents(){


    mm[0] = 0.5;

    for(int ii = 1 ; ii < nn -1 ; ii++){


	mm[ii] = (pp[ii+1] - pp[ii-1]) / (tt[ii+1] - tt[ii-1]);

	}
    mm[nn-1] = 0.5;

    }



int HermiteCatmullSpline::interval(double xx ){
    int ii = (nn   )/ 2 ;

    int pivot = ii;
    int left = 0 ;
    int right = nn - 2;


    pivot = (left  + right )/2;

    while (left!=right ){
	// cerr << "left : " << left << "right : " << right << "pivot " << pivot << endl;
	

	if( xx <=  tt[pivot]  ){right = pivot ; }
	else if(xx > tt[pivot] ){left  = pivot +1 ; }
	else{}
      
	pivot = (left  + right )/2;
	}

    

    return pivot;
    }


bool HermiteCatmullSpline::resetNodes(int nodesNumber){
    

    nn = nodesNumber;
    delete[] mm;
    delete[] pp;
    delete[] tt; 
    mm = new double[nn];
    pp = new double[nn];
    tt = new double[nn];
    reserved = 0 ;

}


bool HermiteCatmullSpline::addNode(double node){

    if(reserved < nn){
	pp[reserved] = node;
	tt[reserved] = reserved;
	reserved++;
	if(reserved == nn)
	    computeTangents();
	return true;
	}
    else{
	return false;
	}


    }
