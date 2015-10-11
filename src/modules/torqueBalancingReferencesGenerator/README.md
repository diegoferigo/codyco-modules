##Module description

This module streams the desired references of the center-of-mass and the postural task stabilized by the
`torqueBalancing` module, either c++ or simulink. 

- For the center of mass, it is possible to stream a sinusoidal reference along a given direction. For instance, the left-and-right of the robot can be achieved by streaming:
~~~
	comDes(t) = A*sin(2*pi*f*t)*[0;1;0]
~~~

- For the postural task, it is possible to stream a number of set points at a given time instants. 


##Module details
###Configuration file
The default configuration file `torqueBalancingRefGen.ini` associated with the robot you specified in 
`YARP_ROBOT_NAME` can be found under the path
~~~
codyco-superbuild/main/codyco-modules/src/modules/torqueBalancingReferencesGenerator/app/robots/YARP_ROBOT_NAME
~~~ 
Users willing to modify the default configuration parameters should create robot-dependent local files as specified in `http://wiki.icub.org/wiki/YARP_ResourceFinder#Robots`. 
For instance, assuming that 
linux users want to modify the configuration file `torqueBalancingRefGen.ini` for `icubGazeboSim`, then they have to 
create (or copy and then modify) the file  `torqueBalancingRefGen.ini` under

~~~
.local/share/yarp/robots/
~~~

This procedure, however, should be cleanly executed by the command `yarp-config` (see `http://www.yarp.it/yarp_yarp-config.html`), 
the description of which is here omitted because of some issues when running this command with
`codyco-superbuild ` configuration files.

Clearly, users willing to change the default configuration parameters in `torqueBalancingRefGen.ini` may modify
the installed `torqueBalancingRefGen.ini` under 

~~~
codyco-superbuild/build/install/share/codyco/robots/YARP_ROBOT_NAME/torqueBalancingRefGen.ini
~~~

Do not forget that further installations of `codyco-superbuild` will overwrite any modification on installed configuration files 
unless the modified files are first copied into the source directory, i.e.

~~~ 
`codyco-superbuild/main/codyco-modules/src/modules/torqueBalancingReferencesGenerator/app/robots/YARP_ROBOT_NAME/torqueBalancingRefGen.ini`
~~~


###Options in torqueBalancingRefGen.ini

The configuration file `torqueBalancingRefGen.ini` can contain several options.
- `name`: module name. Ports will be opened with this name. Default to `torqueBalancingReferencesGenerator`
- `robot`: name of the robot to connect to (`icubGazeboSim` for simulations, `icub` for experiments)
- `period`: controller period in milliseconds. Default is 10ms (100Hz)
- `wbi_config_file`: name (or full path, see ResourceFinder documentation) to the whole body interface initialization file
- `wbi_joint_list`: name of the torque controlled joint list.
- `timeoutBeforeStreamingRefs`: time waited from the module before starting the streaming of references


####The field [REFERENCES]

Assume that one wants to stream only the sinusoidal reference for the center of mass. Then, 
users have to specify the following paramenters 

- `amplitudeInMeters`: amplitude of the sinusoidal oscillation in meters 
- `frequencyInHerz`: frequency of the sinusoidal oscillation in hertz
- `directionOfOscillation`: unit-norm, three element vector specifying the direction of oscillation in (x,y,z).

As for an example, the classical lift-and-right oscillation for the center of mass is obtained by setting:
~~~
amplitudeInMeters 0.03
frequencyInHerz   0.5
directionOfOscillation = (0,1,0)
~~~

Assume that one wants to stream the postural references. Then, users have to specify  a list of lists called  
`postures'. Each list of `postures' is expected to have `1+NDOF` elements, where `NDOF` is the number of 
torque controlled degrees of freedom (usually 23). More precisely, `NDOF` must be equal to the 
number of joints composing the `wbi_joint_list`, which can be found in the `wbi_config_file` 
(see above). The first element of each list of  `postures` is interpreted as the time at which the 
ith posture must be streamed. The ith posture is then specified by the following `NDOF` numbers contained
in the ith list. These numbers are expected to be in radians. For example, assume that `NDOF = 23`, and that 
two robot's postures `q1` and `q2`, which are 23 element vectors of the form `q1_1,q1_2,...,q1_23`, must
be streamed at the time instants `t1` and `t2`. Then, set 

~~~
postures = ((t1,q1),(t2,q2))
~~~

As mentioned before, the robot's postures `q1` and `q2` are interpreted as radians. If they must be interpreted as
degrees, add the option

~~~
posturesInDegrees
~~~

in the configuration file.

####The field [PORTS_INFO]
This field must contain two values:
- `portNameForStreamingComDes`: port that the module creates for streaming the references for the center of mass. Default value `/torqueBalancingRefGen/comDes:o`

- `portNameForStreamingQdes`: port that the module creates for streaming the references for the postural tasks. `/torqueBalancingRefGen/qDes:o`

###Launch procedure
The connection between the ports of this module (referenceGenerator) and those of the torqueBalancing module must be done externally.
For this reason, before starting to stream the references, the module waits a certain time, which is 
configurable in the module's configuration file. Remember that before starting the module, you have to set the environmental variable `YARP_ROBOT_NAME` in the bashrc according to the robot one wants to use (e.g. icubGazeboSim for simulations, or iCubGenova01, iCubParis01, etc. for experiments). Once the module is
launched, the connections between the ports of this module (where references are streamed) and those of the torqueBalancing module (where references are expected) can be established. Assuming that the default names for the ports are set, the connections can be established by running in a terminal

~~~
yarp connect /torqueBalancingRefGen/comDes:o /torqueBalancing/comDes:i
~~~

for connecting the center of mass references and

~~~
yarp connect /torqueBalancingRefGen/qDes:o /torqueBalancing/qDes:i
~~~

for connecting the postural references.