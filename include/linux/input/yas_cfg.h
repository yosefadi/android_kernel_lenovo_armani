/*
 * Copyright (c) 2010 Yamaha Corporation
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef __YAS_CFG_H__
#define __YAS_CFG_H__

#define YAS_ACC_DRIVER_ADXL345              (0)
#define YAS_ACC_DRIVER_ADXL346              (1)
#define YAS_ACC_DRIVER_KXTF9                (2)


/*----------------------------------------------------------------------------*/
/*                               Configuration                                */
/*----------------------------------------------------------------------------*/

#define YAS_ACC_DRIVER                      (YAS_ACC_DRIVER_ADXL346)

/*----------------------------------------------------------------------------*/
/*                   Acceleration Calibration Configuration                   */
/*----------------------------------------------------------------------------*/

#define YAS_DEFAULT_ACCCALIB_LENGTH         (20)

#endif
