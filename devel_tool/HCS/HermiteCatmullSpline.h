





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

int nn;

double *mm; 

double *tt;

 double *pp;



public:

 HermiteCatmullSpline(int,double*, double*);


~HermiteCatmullSpline();

double evaluate(double xx);

int interval(double xx );

    };
