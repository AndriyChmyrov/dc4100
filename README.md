# dc4100
Matlab MEX wrapper written in C/C++ (MS Visual Studio) for Thorlabs DC4100/DC4104 Four Channel LED Drivers (https://www.thorlabs.de/newgrouppage9.cfm?objectgroup_id=3832&pn=DC4100)

Requires Thorlabs software, which can be downloaded for free from: 
https://www.thorlabs.de/software_pages/ViewSoftwarePage.cfm?Code=DC4100

**Syntaxis:**
~~~Matlab
% ---------------------------------------
% Set call type: set Parameter to provided value or execute command, no output value
mircat('setParameterCommand',value); 	

% ---------------------------------------
% Get call type: get Parameter value or get command output
result = mircat('getParameterCommand');	

% ---------------------------------------
% set&get call type: 
% if only Parameter is provided - workds as the "Get call type"
result = mircat('set&getParameterCommand');	
% if Parameter and value are provided - workds as the "Set call type", no output
mircat('set&getParameterCommand', value);	

% ---------------------------------------
% getWithParameter call type: 
% works like Get call typy, but requires an additional parameter (value) - typically a channel number
result = mircat('getWithParameterCommand', value);

% ---------------------------------------
% setORget call type: 
result = mircat('ParameterCommand',value1);
mircat('ParameterCommand',[value1,value2]);
% if value is a scalar - the function returns the value of the Parameter
% if value is a vector - the function sets the value of the Parameter
% Example of setORget call type: 
dc4100('current',1)       - get the current value of the channel 1
dc4100('current',[1,0.5]) - set the current value of the channel 1 to 0.5 Amp
~~~

| Parameters/Commands:		| Call type 		|
| :---						| :----:			|
| brightness				| setORget			|
| close						| get only			|
| current					| setORget			|
| current_max_limit			| setORget			|
| current_user_limit		| setORget			|
| display_brightness		| set&get 			|
| emission					| setORget			|
| error_message				| getWithParameter	|
| error_query				| get only			|
| forward_bias				| getWithParameter	|
| head_info					| getWithParameter	|
| id						| get only			|
| mode						| set&get 			|
| mode_str					| get only			|
| reset						| get only			|
| revision					| get only			|
| selection					| set&get 			|
| selection_str				| get only			|
| self_test					| get only			|
| wavelength				| getWithParameter	|


**Example:**
~~~Matlab
% Get only type calls
dc4100('id')                      % get device information
dc4100('mode')                    % get operation mode as a number
dc4100('mode_str')                % get operation mode as a string: "current", "brightness" or "external"
dc4100('close')                   % close connection to the device

% getWithParameter type calls
dc4100('head_info',1)             % get information about the LED head from the channel 1
dc4100('forward_bias',1)          % get information about the forward bias of the LED from the channel 1

% set&get type calls
dc4100('mode')                    % get operation mode as a number
dc4100('mode','current')          % set operation mode to "constant current"
dc4100('display_brightness')      % get display brightness (0.5 = 50% intensity)
dc4100('display_brightness',0.25) % set display brightness to 25%

% setORget type calls
dc4100('current',1)               % get the current value (in Amp) of the channel 1
dc4100('current',[1,0.5])         % set the current value of the channel 1 to 0.5 Amp
dc4100('emission',1)              % get emission status of the channel 1
dc4100('emission',[3,1])          % set emission status of the channel 3 to ON
~~~