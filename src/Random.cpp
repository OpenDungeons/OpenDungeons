/*!
 * \file   Random.cpp
 * \date   26 April 2011
 * \author andrewbuck, StefanP.MUC
 * \brief  Offers some random number generating functions
 */

#ifdef __MINGW32__
#ifndef mode_t
#include <sys/types.h>
#endif //mode_t
#endif //mingw32
#include <cmath>
#include <ctime>
#include <semaphore.h>

#include "Random.h"

namespace Random
{
    namespace
    {
    sem_t randomGeneratorLockSemaphore;
    unsigned long myRandomSeed;
    const unsigned long MAX = 32768;

    unsigned long randgen()
    {
        sem_wait(&randomGeneratorLockSemaphore);
        myRandomSeed = myRandomSeed * 1103515245 + 12345;
        unsigned long returnVal = (unsigned int) (myRandomSeed / 65536) % MAX;
        sem_post(&randomGeneratorLockSemaphore);

        return returnVal;
    }

    /*! \brief uniformly distributed number [0;1)
     *
     */
    double uniform()
    {
        return randgen() * 1.0 / static_cast<double>(MAX);
    }

    /*! \brief uniformly distributed number [0;hi)
     *
     */
    double uniform(double hi)
    {
        return uniform() * hi;
    }

    /*! \brief uniformly distributed number [lo;hi)
     *
     */
    double uniform(double lo, double hi)
    {
        return uniform() * (hi - lo) + lo;
    }

    /*! \brief random bit
     *
     */
    int randint()
    {
        return uniform() > 0.5
                ? 1
                : 0;
    }

    /*! \brief random integer [0;hi]
     *
     */
    int randint(int hi)
    {
        return static_cast<signed>(uniform() * (hi + 1));
    }

    /*! \brief random integer [lo;hi]
     *
     */
    int randint(int lo, int hi)
    {
        return static_cast<signed>(uniform() * (hi - lo + 1) + lo);
    }

    /*! \brief random unsigned integer [0;hi]
     *
     */
    unsigned int randuint(unsigned int hi)
    {
        return static_cast<unsigned int>(uniform() * (hi + 1));
    }

    /*! \brief random unsigned integer [lo;hi]
     *
     */
    unsigned int randuint(unsigned int lo, unsigned int hi)
    {
        return static_cast<unsigned int>(uniform() * (hi - lo + 1) + lo);
    }

    } /* UNNAMED NAMESPACE */

/*! \brief initializes the semphore and seeds the generator
 *
 */
void initialize()
{
    sem_init(&randomGeneratorLockSemaphore, 0, 1);
    myRandomSeed = time(0);
}

/*! \brief generate a random double
 *
 *  \param min, max One or both can be negative
 *
 *  \return a double between the lower number entered and the higher
 *          number entered
 */
double Double(double min, double max)
{
    if (min > max)
    {
        double temp = min;
        min = max;
        max = temp;
    }

    return uniform(min, max);
}

/*! \brief generate a random int
 *
 *  \param min, max One or both can be negative
 *
 *  \return an integer between the lower number entered and the higher
 *          number entered
 */
int Int(int min, int max)
{
    if (min > max)
    {
        int temp = min;
        min = max;
        max = temp;
    }

    return randint(min, max);
}

/*! \brief generate a random unsigned int
 *
 *  \param min, max One or both can be negative
 *
 *  \return a unsigned int between the lower number entered and the higher
 *          number entered
 */
unsigned int Uint(unsigned int min, unsigned int max)
{
    if (min > max)
    {
        unsigned int temp = min;
        min = max;
        max = temp;
    }

    return randuint(min, max);
}

/*! \brief generates a gaussian distributed random double
 *
 *  \return a gaussian distributed random double value in [-1,1]
 */
double gaussianRandomDouble()
{
    return sqrt(-2.0 * log(Double(0.0, 1.0)))
            * cos(2.0 * M_PI * Double(0.0, 1.0));
}

} /* NAMESPACE RANDOM*/
