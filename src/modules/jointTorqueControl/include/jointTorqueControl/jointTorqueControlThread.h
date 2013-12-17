/* 
 * Copyright (C) 2013 CoDyCo
 * Author: Daniele Pucci	
 * email:  daniele.pucci@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

#ifndef __JOINT_TORQUE_CONTROL_THREAD
#define __JOINT_TORQUE_CONTROL_THREAD

#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <cstdlib>
#include <stdio.h>
#include <time.h>

#include <yarp/os/RateThread.h>
#include <Eigen/Core>                               // import most common Eigen types
#include <wbi/wbi.h>
#include <paramHelp/paramHelperServer.h>
#include <paramHelp/paramHelperClient.h>

#include <jointTorqueControl/jointTorqueControlConstants.h>


using namespace yarp::os;
using namespace std;
using namespace paramHelp;
using namespace wbi;
using namespace Eigen;

namespace jointTorqueControl
{

enum ControlStatus {CONTROL_ON, CONTROL_OFF};
/** thread...
*/
class jointTorqueControlThread: public RateThread, public ParamValueObserver, public CommandObserver
{
    string              name;
    string              robotName;
    ParamHelperServer   *paramHelper;
    wholeBodyInterface  *robot;
    
    int             printCountdown;     ///< counter for printing every PRINT_PERIOD ms
    bool            mustStop;
    ControlStatus   status;
    double		    oldTime;  
    int             monitoredJointId;
    VectorNi        activeJointsOld;    ///< value of the vector activeJoints before it was changed
    VectorNd        dqSign;             ///< approximation of the sign of the joint vel
    VectorNd        pwmMeas;            ///< measured motor PWMs

    // Thread parameters
	VectorNi 	activeJoints;	// Vector of nDOF integers representing the joints to control  (1: active, 0: inactive) 
    VectorNd	dq;				// Joint velocities 
    VectorNd 	kt;				// Vector of nDOF floats ( see Eq. (1) )"), 
    VectorNd 	kvp;			// Vector of nDOF floats ( see Eq. (2) )"), 
    VectorNd	kvn;			// Vector of nDOF floats ( see Eq. (2) )"), 
    VectorNd	kcp;			// Vector of nDOF floats ( see Eq. (2) )"), 
    VectorNd	kcn;			// Vector of nDOF floats ( see Eq. (2) )"), 
    VectorNd	ki;				// Vector of nDOF floats representing the position gains ( see Eq. (x) )"), 
    VectorNd	kp;				// Vector of nDOF floats representing the integral gains ( see Eq. (x) )"), 
    VectorNd	coulombVelThr;	///< Vector of nDOF floats representing the joint vel (deg/s) at which Coulomb friction is completely compensated
    VectorNd	etau;			// Errors between actual and desired torques 
    VectorNd	tau;			// Vector of nDOF floats representing the steepnes       ( see Eq. (x) )"), 
    VectorNd	tauD;			// Vector of nDOF floats representing the steepnes       ( see Eq. (x) )"), 
    VectorNd	tauM;			// Measured torques, 
    VectorNd	integralState;	// Vector of nDOF floats representing the steepnes       ( see Eq. (x) )"), 
    VectorNd	motorVoltage;	// Vector of nDOF positive floats representing the tensions' bounds (|Vm| < Vmax"), 
    VectorNd	Vmax;			// Vector of nDOF positive floats representing the tensions' bounds (|Vm| < Vmax"),     
    int			sendCommands;
	string      monitoredJointName;     ///< name of the monitored joint
	
    //monitored variables
    struct
    {
        double tauMeas;
        double tauDes;
        double tadDesPlusPI;
        double dq;
        double dqSign;
        double pwmDes;
        double pwmMeas;
        double pwmFF;
        double pwmFB;
        double pwmTorqueFF;
        double pwmFrictionFF;
    } monitor;
	
    // Input streaming parameters

    // Output streaming parameters
    
    enum MsgType {MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR};
    void sendMsg(const string &msg, MsgType msgType=MSG_INFO) ;

    void prepareMonitorData();
		
    /** Reset the integral state of the specified joint. */
    void resetIntegralState(int j);
	
    /** If activeTorque is true set all the active joints to open loop control.
     * If activeTorque is false set all the joints to position control. */
	void setControlModePWMOnJoints(bool activeTorque);
	
    /** Convert the global id contained in the specified Bottle into a local id to access
     * the vector of this thread. The Bottle b may either contain an integer id or the name
     * of the joint.
     * @param b Bottle containing the global id.
     * @return The local id, -1 if nothing was found. */
    int convertGlobalToLocalJointId(const yarp::os::Bottle &b);

    /** Update the variable jointMonitor based on the value of the variable jointMonitorName. 
     * @return True if the current monitored joint name was recognized, false otherwise. */
    bool updateJointToMonitor(); 

    /** Method called every time the parameter activeJoints changes value. It resets
     * the integral state of the joints that are activated and change the control mode
     * of the joints that are activated/deactivated. */
    bool activeJointsChanged();
    
    bool readRobotStatus(bool);

public:	
    
    /* If you define a structure having members of fixed-size vectorizable Eigen types, you must overload 
     * its "operator new" so that it generates 16-bytes-aligned pointers. Fortunately, Eigen provides you 
     * with a macro EIGEN_MAKE_ALIGNED_OPERATOR_NEW that does that for you. */
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    jointTorqueControlThread(int period, string _name, string _robotName, ParamHelperServer *_ph, wholeBodyInterface *_wbi);
	
    bool threadInit();	
    void run();
    void startSending();
    void stopSending();

    void threadRelease(){}

//     void stop(){ mustStop=true; Thread::stop(); }

    /** Callback function for parameter updates. */
    void parameterUpdated(const ParamProxyInterface *pd);
    /** Callback function for rpc commands. */
    void commandReceived(const CommandDescription &cd, const Bottle &params, Bottle &reply);
};

} // end namespace

#endif
