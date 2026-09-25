# mkpass config

The idea is to let the user make some default choices permanent in a config file.


Scope:
1. Implement toml config parsing and saving functionality: ~/.config/mkpass/mkpass.conf
2. Implement all supported evvironment variables (except for password and service) as config options
3. Implement the "Settings" dialog in GUI and Android versions.


## The cli conig command
The mkpass cli config comand must resemble the git config comand.
mkpass config get - to get a value
mkpass config set - to set a value
mkpass config unset - to remove a value
mkpass config print - to print the config


## The settings dialog

The settings dialog must provide capabilities to change the defaults for every variable.
A table with all available variables with 3 columns:
  - a tickbox to enable/disable the variable. A disabled variable should be in grey.
  - the variable name
  - the variable value


## Hide the "Old Algoithm" by default
Only show the old algorithm in the algorithm selection list:
  - either if it's enabled in the config or env variable "enable_old_algorithm"
  - or when an existing service selected which is using the old algorithm

In the second case we support maintainig exising services which are using the old algorithm, but we don't allow creating new services with the old algorithm.

enable_old_algorithm must be false by default.
