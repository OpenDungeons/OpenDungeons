/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HERMITECATMULLSPLINE_H_
#define HERMITECATMULLSPLINE_H_

//! \brief A class permitting to define a serie of control points setting up a spline.
// For an explanation of what are Catmull Rom Splines, have a look here for instance:
// http://www.mvps.org/directx/articles/catmull/
class HermiteCatmullSpline
{
public:
    HermiteCatmullSpline(int nn_in = 1);

    HermiteCatmullSpline(int nn_in, double* pp_in, double* tt_in);

    ~HermiteCatmullSpline();

    void resetNodes(int nn);

    bool addNode(double node);

    inline int getNN()
    { return mNn; }

    double evaluate(double xx);

private:
    struct Point
    {
        double xx;
        double yy;
        double zz;
    };

    int mNn;

    double* mMm; 

    double* mTt;

    double* mPp;

    int mReserved;

    double h00(double xx);

    double h01(double xx);

    double h10(double xx);

    double h11(double xx);

    double evaluate_aux(double tk, double tk_pp1,
                        double mk, double mk_pp1,
                        double pk, double pk_pp1,
                        double xx);

    void computeTangents();

    int interval(double);
};

#endif // HERMITECATMULLSPLINE_H_
