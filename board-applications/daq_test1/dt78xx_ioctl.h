/*
 *	IOCTLs used to communicate from user application to DT78xx kernel driver
 *  
 *  (C) Copyright (c) 2014 Data Translation Inc
 *                    www.datatranslation.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _DT78XX_IOCTL_H_
#define _DT78XX_IOCTL_H_

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#ifdef	__cplusplus
extern "C"
{
#endif
#define DT78XX_IOCTL            (0xf3)
#define READ_BOARD_INFO         (1)
#define READ_SUBSYS_CFG         (2)
#define WRITE_SUBSYS_CFG        (3)
#define GET_SINGLE_VALUE        (4)
#define SET_SINGLE_VALUE        (5)
#define START_SUBSYS            (6)
#define STOP_SUBSYS             (7)
#if 0
/*
 * Data stricture used for get/set subsystem configuration
 */ 
typedef struct __attribute__ ((__packed__))
{
    subsystem_id_t      id;         //MUST BE FIRST
    subsystem_config_t  cfg;    
} ioctl_subsystem_config_t;

/*
 * Data structure used to get/set single value
 */
typedef struct __attribute__ ((__packed__))
{
    subsystem_id_t      id;         //MUST BE FIRST
    single_value_t      single_val;
}ioctl_single_value_t;

/*
 *  IOCTL command to read board information
 *  Input :
 *      pointer to board_info_t
 *  Return :
 *      0 = success; board_info_t filled in on success
 *      <0 = failure
 */
#define IOCTL_READ_BOARD_INFO	\
            _IOR(DT78XX_IOCTL,  READ_BOARD_INFO, board_info_t)
    
/*
 *  IOCTL command to read subsystem configuration
 *  Input :
 *      pointer to ioctl_subsystem_config_t with all members of structure id
 *      initialized
 *  Return :
 *      0 = success; subsystem_config_t filled, with the configuration for 
 *          the specified sub system type
 *      <0 = failure
 */
#define IOCTL_READ_SUBSYS_CFG	\
            _IOWR(DT78XX_IOCTL, READ_SUBSYS_CFG, ioctl_subsystem_config_t)
    
/*
 *  IOCTL command to write subsystem configuration
 *  Input :
 *      pointer to ioctl_subsystem_config_t with all members of structure id
 *      initialized in addition to other members
 *  Return :
 *      0 = success; specified subsystem has been configured
 *     <0 = failure
 */
#define IOCTL_WRITE_SUBSYS_CFG	\
            _IOW(DT78XX_IOCTL, WRITE_SUBSYS_CFG, ioctl_subsystem_config_t)
    
/*
 *  IOCTL command to read a single value from a subsystem
 *  Input :
 *      pointer to ioctl_single_value_t with all members of structure id
 *      initialized
 *  Return :
 *      0 = success; single_value_t has value from susbsystem
 *     <0 = failure
 */
#define IOCTL_GET_SINGLE_VALUE	\
            _IOWR(DT78XX_IOCTL, GET_SINGLE_VALUE, ioctl_single_value_t)
    
/*
 *  IOCTL command to write a single value from a subsystem
 *  Input :
 *      pointer to ioctl_single_value_t with all members of structure id
 *      initialized to a valid value in addition to other members
 *  Return :
 *      0 = success; subsytem has been updated with value
 *     <0 = failure
 */
#define IOCTL_SET_SINGLE_VALUE	\
            _IOW(DT78XX_IOCTL, SET_SINGLE_VALUE, ioctl_single_value_t)
#endif    
/*
 *  IOCTL command to start a subsystem
 *  Input :
 *      pointer to subsystem_id_t type specifying the subsystem type and 
 *      subsystem element number
 *  Return :
 *      0 = success; subsytem has started
 *     <0 = failure
 */
#define IOCTL_START_SUBSYS	\
            _IOW(DT78XX_IOCTL, START_SUBSYS, int)
    
/*
 *  IOCTL command to stop a subsystem
 *  Input :
 *      pointer to subsystem_id_t type specifying the subsystem type and 
 *      subsystem element number
 *  Return :
 *      0 = success; subsytem has been stopped
 *     <0 = failure
 */
#define IOCTL_STOP_SUBSYS	\
            _IOW(DT78XX_IOCTL, STOP_SUBSYS, int)
    
#ifdef	__cplusplus
}
#endif

#endif //_DT78XX_IOCTL_H_