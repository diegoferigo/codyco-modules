/* 
 * Copyright (C) 2013 CoDyCo
 * Author: Andrea Del Prete
 * email:  andrea.delprete@iit.it
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

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string.h>

#include <wbiIcub/wholeBodyInterfaceIcub.h>
#include <iCub/skinDynLib/common.h>

#include "motorFrictionIdentificationLib/motorFrictionExcitationParams.h"
#include "motorFrictionExcitation/motorFrictionExcitationModule.h"

using namespace yarp::dev;
using namespace paramHelp;
using namespace wbiIcub;
using namespace iCub::skinDynLib;
using namespace motorFrictionExcitation;

MotorFrictionExcitationModule::MotorFrictionExcitationModule()
{
    ctrlThread      = 0;
    robotInterface  = 0;
    paramHelper     = 0;
    period          = 20;
}
    
bool MotorFrictionExcitationModule::configure(ResourceFinder &rf)
{		
    //--------------------------PARAMETER HELPER--------------------------
    paramHelper = new ParamHelperServer(motorFrictionExcitationParamDescr, PARAM_ID_SIZE, motorFrictionExcitationCommandDescr, COMMAND_ID_SIZE);
    paramHelper->linkParam(PARAM_ID_MODULE_NAME, &moduleName);
    paramHelper->linkParam(PARAM_ID_CTRL_PERIOD, &period);
    paramHelper->linkParam(PARAM_ID_ROBOT_NAME, &robotName);
    paramHelper->registerCommandCallback(COMMAND_ID_HELP, this);
    paramHelper->registerCommandCallback(COMMAND_ID_QUIT, this);

    // Read parameters from configuration file (or command line)
    Bottle initMsg;
    paramHelper->initializeParams(rf, initMsg);
    printBottle(initMsg);

    // Open ports for communicating with other modules
    if(!paramHelper->init(moduleName)){ fprintf(stderr, "Error while initializing parameter helper. Closing module.\n"); return false; }
    rpcPort.open(("/"+moduleName+"/rpc").c_str());
    setName(moduleName.c_str());
    attach(rpcPort);

    //--------------------------WHOLE BODY INTERFACE--------------------------
    robotInterface = new icubWholeBodyInterface(moduleName.c_str(), robotName.c_str());
    robotInterface->addJoints(ICUB_MAIN_JOINTS);
    robotInterface->addEstimate(ESTIMATE_FORCE_TORQUE, LocalId(RIGHT_LEG,0));  // right get ft sens
    robotInterface->addEstimate(ESTIMATE_FORCE_TORQUE, LocalId(LEFT_LEG,0));   // left leg ft sens
    if(!robotInterface->init()){ fprintf(stderr, "Error while initializing whole body interface. Closing module\n"); return false; }

    //--------------------------CTRL THREAD--------------------------
    ctrlThread = new MotorFrictionExcitationThread(moduleName, robotName, period, paramHelper, robotInterface, rf);
    if(!ctrlThread->start()){ fprintf(stderr, "Error while initializing motorFrictionExcitation control thread. Closing module.\n"); return false; }
    
    fprintf(stderr,"MotorFrictionExcitation control started\n");
	return true;
}

bool MotorFrictionExcitationModule::respond(const Bottle& cmd, Bottle& reply) 
{
    paramHelper->lock();
	if(!paramHelper->processRpcCommand(cmd, reply)) 
	    reply.addString( (string("Command ")+cmd.toString().c_str()+" not recognized.").c_str());
    paramHelper->unlock();

    // if reply is empty put something into it, otherwise the rpc communication gets stuck
    if(reply.size()==0)
        reply.addString( (string("Command ")+cmd.toString().c_str()+" received.").c_str());
	return true;	
}

void MotorFrictionExcitationModule::commandReceived(const CommandDescription &cd, const Bottle &params, Bottle &reply)
{
    switch(cd.id)
    {
    case COMMAND_ID_HELP:   
        paramHelper->getHelpMessage(reply);     
        break;
    case COMMAND_ID_QUIT:   
        stopModule(); 
        reply.addString("Quitting module.");    
        break;
    }
}

bool MotorFrictionExcitationModule::interruptModule()
{
    return true;
}

bool MotorFrictionExcitationModule::close()
{
	//stop threads
    if(ctrlThread){     ctrlThread->suspend();      ctrlThread->stop();     delete ctrlThread;      ctrlThread = 0;     }
    if(paramHelper){    paramHelper->close();       delete paramHelper;     paramHelper = 0;    }
    if(robotInterface)
    { 
        bool res=robotInterface->close();    
        if(res)
            printf("Error while closing robot interface\n");
        delete robotInterface;  
        robotInterface = 0; 
    }

	//closing ports
    rpcPort.interrupt();
	rpcPort.close();

    printf("[PERFORMANCE INFORMATION]:\n");
    printf("Expected period %d ms.\nReal period: %3.1f+/-%3.1f ms.\n", period, avgTime, stdDev);
    printf("Real duration of 'run' method: %3.1f+/-%3.1f ms.\n", avgTimeUsed, stdDevUsed);
    if(avgTimeUsed<0.5*period)
        printf("Next time you could set a lower period to improve the controller performance.\n");
    else if(avgTime>1.3*period)
        printf("The period you set was impossible to attain. Next time you could set a higher period.\n");

    return true;
}

bool MotorFrictionExcitationModule::updateModule()
{
    if (ctrlThread==0)
    {
        printf("ControlThread pointers are zero\n");
        return false;
    }

    ctrlThread->getEstPeriod(avgTime, stdDev);
    ctrlThread->getEstUsed(avgTimeUsed, stdDevUsed);     // real duration of run()
//#ifndef NDEBUG
    if(avgTime > 1.3 * period)
    {
        printf("[WARNING] Control loop is too slow. Real period: %3.3f+/-%3.3f. Expected period %d.\n", avgTime, stdDev, period);
        printf("Duration of 'run' method: %3.3f+/-%3.3f.\n", avgTimeUsed, stdDevUsed);
    }
//#endif

    return true;
}
