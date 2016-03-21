/*!
 * \file   Random.h
 * \date   26 April 2011
 * \author andrewbuck, StefanP.MUC
 * \brief  Offers some random number generating functions
 *
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef RANDOM_H_
#define RANDOM_H_

namespace Random
{
    //! \brief initializes the semaphore and seeds the generator
    void initialize();

    /*! \brief generate a random double
     *
     *  \param min, max One or both can be negative
     *
     *  \return a double between the lower number entered and the higher
     *          number entered
     */
    double Double(double min, double max);

    /*! \brief generate a random int
     *
     *  \param min, max One or both can be negative
     *
     *  \return an integer between the lower number entered and the higher
     *          number entered
     */
    int Int(int min, int max);

    /*! \brief generate a random unsigned int
     *
     *  \param min, max One or both can be negative
     *
     *  \return a unsigned int between the lower number entered and the higher
     *          number entered
     */
    unsigned int Uint(unsigned int min, unsigned int max);

    /*! \brief generates a gaussian distributed random double
     *
     *  \return a gaussian distributed random double value in [-1,1]
     */
    double gaussianRandomDouble();
}

#endif // RANDOM_H_
