# Guide 

Readme shows hardware connections and how to open the project software correctly in mtb.

## Hardware connections

### Car subsystem hardware connections

![image](https://github.com/user-attachments/assets/ce5b7b2b-da92-4254-a9d9-de9d55d74974)

### Controller subsystem hardware connections

![image](https://github.com/user-attachments/assets/fc9c7adf-aa9e-4eb3-8520-71af7672a98e)

## How to open projects

In mtb for both the car and controller just create a new application in a different workspace then the git repo, like the mtw. Then choose correct board. Then press brows application 
instead of choosing one of the existing examples. 

![image](https://github.com/user-attachments/assets/62e82b02-0f2b-42b2-9360-eada171aa92a)

When importing select the folder of the project from the git repo and open the project like that. MTBshared should be automatically generated.
If in the car project I2CHWBUS errors appear do the following. Enter device configurator and under periphirals and communication tick on SCB3 and call it "I2CBUS". Then copy the settings:

![image](https://github.com/user-attachments/assets/a2163f93-05d3-40b6-8b9e-f1d8ab11f718)
