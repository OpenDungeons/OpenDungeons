

#ifndef HERMITECATMULLSPLINE_H_
#define HERMITECATMULLSPLINE_H_



class HermiteCatmullSpline{


struct Point{

    double xx;
    double yy;
    double zz;


    };


double h00(double xx );

double h01(double xx );

double h10(double xx );

double h11(double xx );

double evaluate_aux( double , double  , double , double  , double , double , double );

double computeTangents();

int interval(double  );


int nn;

double *mm; 

double *tt;

double *pp;

int reserved;

public:

bool resetNodes(int nn);
bool addNode(double );
inline int getNN(){return nn;};

 HermiteCatmullSpline(int nn_in= 1);

 HermiteCatmullSpline(int,double*, double*);


~HermiteCatmullSpline();

double evaluate(double xx);



    };


#endif /* HERMITECATMULLSPLINE_H_ */
