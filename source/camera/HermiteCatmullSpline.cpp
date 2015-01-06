/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "camera/HermiteCatmullSpline.h"

#include <iostream>

HermiteCatmullSpline::HermiteCatmullSpline(int nn_in):
    mNn(nn_in)
{
    mMm = new double[mNn];
    mPp = new double[mNn];
    mTt = new double[mNn];
    computeTangents();
}

HermiteCatmullSpline::HermiteCatmullSpline(int nn_in, double* pp_in, double* tt_in):
    mNn(nn_in),
    mTt(tt_in),
    mPp(pp_in)
{
    mMm = new double[mNn];
    computeTangents();
}

HermiteCatmullSpline::~HermiteCatmullSpline()
{
    delete[] mMm;
    delete[] mPp;
    delete[] mTt;
}

double HermiteCatmullSpline::h00(double xx)
{
    return (1 + 2 * xx) + (1 - xx) * (1 - xx);
}

double HermiteCatmullSpline::h10(double xx)
{
    return xx * (1 - xx) * (1 - xx);
}

double HermiteCatmullSpline::h01(double xx)
{
    return xx * xx * (3 - 2 * xx);
}

double HermiteCatmullSpline::h11(double xx)
{
    return xx * xx * (xx - 1);
}

double HermiteCatmullSpline::evaluate_aux(double tk, double tk_pp1, double mk, double mk_pp1, double pk, double pk_pp1, double xx)
{
    double mTt = (xx - tk) / (tk_pp1 - tk);
    return h00(mTt) * pk + h10(mTt) * (tk_pp1 - tk) * mk + h10(mTt) * pk_pp1 + h01(mTt) * (tk_pp1 - tk) * mk_pp1;
}

double HermiteCatmullSpline::evaluate(double xx)
{
    int ii = interval(xx);

    if(ii != -1)
        return evaluate_aux(mTt[ii], mTt[ii + 1], mMm[ii], mMm[ii + 1], mPp[ii], mPp[ii + 1], xx) / 2;
    else
        return 0;
}

void HermiteCatmullSpline::computeTangents()
{
    mMm[0] = 0.5;

    for(int ii = 1; ii < mNn - 1; ++ii)
    {
        mMm[ii] = (mPp[ii + 1] - mPp[ii - 1]) / (mTt[ii + 1] - mTt[ii - 1]);
    }

    mMm[mNn - 1] = 0.5;
}

int HermiteCatmullSpline::interval(double xx)
{
    int ii = mNn / 2;

    int pivot = ii;
    int left = 0;
    int right = mNn - 2;

    pivot = (left + right) / 2;

    while (left != right)
    {
        // cerr << "left : " << left << "right : " << right << "pivot " << pivot << endl;

        if(xx <= mTt[pivot])
        {
            right = pivot;
        }
        else if(xx > mTt[pivot])
        {
            left = pivot + 1;
        }

        pivot = (left + right) / 2;
    }

    return pivot;
}

void HermiteCatmullSpline::resetNodes(int nodesNumber)
{
    mNn = nodesNumber;
    delete[] mMm;
    delete[] mPp;
    delete[] mTt;
    mMm = new double[mNn];
    mPp = new double[mNn];
    mTt = new double[mNn];
    mReserved = 0;
}

bool HermiteCatmullSpline::addNode(double node)
{
    if(mReserved < mNn)
    {
        mPp[mReserved] = node;
        mTt[mReserved] = static_cast<double>(mReserved) / 2;
        ++mReserved;

        if(mReserved == mNn)
            computeTangents();

        return true;
    }

    return false;
}
