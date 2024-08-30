# UAV-Optimization
This github repo is a summary of my UROP experience during the summer of 2024 under the supervision of Dr Stefan Vlaski.
## Project Introduction
My research project is around the use of UAVs as edge computing devices for end users. For specfically, I explored how to optimise UAV trajectory in order to make them more energy efficient at being edge computing devices. 
## Problem Formation
The optimization Problem I need to solve is as follow:
There is a single UAV and multiple mobile users. The UAV is being given a fixed starting position and a ending position. The UAV flys at a fixed height with constrains on max velocity, acceration so that it mimic the actual UAV. The mobile users are fixed, and they have compututaionally heavy tasks that need to be processed. Both the UAV and the mobile users has onboard computational power. The end user can offload there tasks to the UAV to safe energy. The aim of is to minimise the total energy consumption of the UAV and mobile users while optimising the trajectory of the drone.
## Model Creation
Before optimization, a realistic model of the drone's dynamics, the end users, the communication model and the computational model is required. A simulation environment is also required for them to interact. It's time consumping to create a model from scratch, and there are very good open source models available that include UAV dynamics and communication. One such model that I use is called UB-ANC. It uses qgroundcontrol for UAV modelling and discrete event simulator ns-3 for communication modeling. This model is not ready to use as it lacks the end users and the computational model. So I adjusted the model by adding a commuincation model and computational model. With the model ready, the next step is to optimise.
## Optimization
Since the optimization is based on this model, a model based optimization technique called Bayesian Optimization is used. There are several reasons to choose this method. First, the model is a black box which no closed form is know. Secondly, the model is expensive to evaluate as the simulation runs in real time. Baysian optimizatin is particularly useful in this case because it creates a surrogate model to approximate the objective function and uses an acquisition function to guide the exploration of the input space.
