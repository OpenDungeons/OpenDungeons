/*!
 * \file   Random.h
 * \date   26 April 2011
 * \author andrewbuck, StefanP.MUC
 * \brief  Offers some random number generating functions
 */

#ifndef RANDOM_H_
#define RANDOM_H_

namespace Random
{
    void initialize();

    double Double(double min, double max);
    int Int(int min, int max);
    unsigned int Uint(unsigned int min, unsigned int max);
    double gaussianRandomDouble();
}

#endif /* RANDOM_H_ */
