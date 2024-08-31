# UAV-Optimization
This github repo is a summary of my UROP experience during the summer of 2024 under the supervision of Dr Stefan Vlaski.
## Project Introduction
My research project is around the UAV-assisted mobile edge computing systems. More specifically, I explored how to optimise UAV trajectory in order to make them more energy efficient at providing edge computing services. 
## Problem Formation
The optimization problem I need to solve is as follow:
There is a single UAV and multiple mobile users. The UAV flys at a fixed height with constrains including max velocity and acceration so that it can mimic the behaviour of an actual UAV. The mobile users are fixed, and they have compututationally heavy tasks that need to be processed. Both the UAV and the mobile users has onboard computational power. The mobile users can offload there tasks to the UAV to safe energy. The UAV is being given a fixed starting position and a ending position. The aim of is to minimise the total energy consumption of the UAV and mobile users while optimising the trajectory of the drone.
## Model Creation
Before optimization, a realistic model of the UAV's dynamics, the mobile users, the communication model and the computational model are required. A simulation environment is also required for them to interact with each other. It's time consumping to create a model from scratch, and there are very good open source models available that include UAV dynamics and communication. The model that I use is called [UB-ANC](https://github.com/jmodares/UB-ANC-Emulator). It uses qgroundcontrol for UAV modelling and discrete event simulator ns-3 for communication modeling. This model is not ready to use as it lacks the end users and the computational model. So I adjusted the model by adding a commuincation model and computational model. With the model ready, the next step is to optimise.
### Communication Model
The model for communication is a line of sight communication with simple free-space path-loss model. A constant speed propagation delay model and Friis Propagation loss model fron ns-3 is used.
### Computational Model
The energy of computation is modelled as: energy proportional to clock frequency cubed with constant as effective switched capacitance
### Model Input and Output
Inputs: The inputs to your model defines the trajectory of the UAV which include distinct waypoints along the start and end position

Output: The model outputs the total energy consumption for a given trajectory. The QoS could be a constraint, such as communication quality, latency, or signal strength between the drone and the mobile devices.
## Contradicting Objectives

## Optimization
Since the optimization is based on this model, a model based optimization technique called Bayesian Optimization is used. There are several reasons to choose this method. First, the model is a black box which no closed form is know. Secondly, the model is expensive to evaluate as the simulation runs in real time. Baysian optimizatin is particularly useful in this case because it creates a surrogate model to approximate the objective function and uses an acquisition function to guide the exploration of the input space.
### Objective Function and Constrains
Objective: Minimize the total energy consumption of the drone 
Constrains: The quality of service constrain is applied to the problem as a soft constrain (incorporated into the objective function as a penalty)


