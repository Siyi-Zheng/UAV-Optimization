# UAV Trajectory Optimization for Mobile Edge Computing
This github repo is a summary of my UROP experience during the summer of 2024 under the supervision of Dr Stefan Vlaski.
## Project Introduction
The project focuses on optimizing Unmanned Aerial Vehicle (UAV) trajectories for energy efficiency in UAV-assisted mobile edge computing systems. The goal is to explore how UAVs can minimize energy consumption while providing computational offloading services to mobile users with heavy computational demands.
## Problem Formation
The optimization problem involves a single UAV assisting multiple mobile users with computational offloading. The UAV operates at a fixed altitude, subject to constraints such as maximum velocity and acceleration to simulate realistic UAV behavior. The mobile users are stationary and have computational tasks that need processing. Both the UAV and mobile users possess onboard computational capabilities, but the users can offload tasks to the UAV to conserve energy.

The UAV's objective is to minimize the total energy consumption of both the UAV and the users while following an optimized trajectory between a fixed starting and ending position. This involves a trade-off between minimizing the UAV’s energy used in flight and optimizing its trajectory to enhance communication efficiency with the users.
## Model Development
Before optimization, a realistic model of the UAV's dynamics, the mobile users, the communication model and the computational model are required. A simulation environment is also required for them to interact with each other. It's time consumping to create a model from scratch, and there are very good open source models available that include UAV dynamics and communication. The model that I use is called [UB-ANC](https://github.com/jmodares/UB-ANC-Emulator). It uses qgroundcontrol for UAV modelling and discrete event simulator ns-3 for communication modeling. This model is not ready to use as it lacks the end users and the computational model. So I adjusted the model by adding a commuincation model and computational model. With the model ready, the next step is to optimise.
### Dynamics Model
The dynamics of the UAV are modeled using the QGroundControl library, which is primarily used for real-world UAV control. This ensures a high-fidelity simulation of UAV flight characteristics, sufficient for the optimization process.
### Communication Model
The communication between the UAV and mobile users is modeled using the ns-3 simulator, implementing a line-of-sight communication channel with the Friis propagation model for signal loss and a constant speed propagation delay model. Given that UAVs typically operate above ground-level obstructions, the line-of-sight assumption accurately reflects real-world conditions.
### Computational Model
The energy consumption of computational tasks is modeled based on the relationship between power consumption and clock frequency. Power is assumed to scale with the square of the clock frequency, with the constant of proportionality being the effective switched capacitance, determined by the characteristics of the processor.
### Model Input and Output
Inputs: The inputs to your model defines the trajectory of the UAV which include distinct waypoints along the start and end position

Output: The model outputs the total energy consumption for a given trajectory. The QoS could be a constraint, such as communication quality, latency, or signal strength between the drone and the mobile devices.
## Contradicting Objectives
In this problem, a straight-line trajectory minimizes the UAV's movement energy but may place it far from some mobile users, increasing the energy required for task offloading due to weaker communication signals. On the other hand, flying closer to each mobile user improves communication efficiency, reducing offloading energy but increasing the UAV's energy consumption due to longer and more complex flight paths. These two objectives—minimizing flight energy and minimizing communication energy—are in direct conflict, requiring a trade-off between them. An optimal solution balances the UAV’s trajectory to reduce both flight and communication energy. The goal is to find a path that minimizes the combined energy of the UAV and the mobile users.
## Optimization
Given the complexity of the problem, Bayesian Optimization is employed to find an optimal solution. This method is well-suited for problems with black-box models where the objective function is unknown or non-linear, and evaluations are computationally expensive.

Bayesian Optimization constructs a surrogate model—typically a Gaussian Process—that approximates the true objective function based on previous evaluations. Instead of directly evaluating the computationally expensive simulation at each iteration, the surrogate model is used to predict likely outcomes. An acquisition function then guides the selection of the next input (UAV trajectory) by balancing exploration (searching uncertain areas) and exploitation (focusing on areas likely to yield better results). This allows for efficient convergence to an optimal solution with fewer evaluations, making it ideal for UAV trajectory optimization and task offloading scenarios.

## Conclusion
This project successfully integrates UAV dynamics, communication, and computation models into a simulation environment to optimize UAV trajectories for energy efficiency in mobile edge computing systems. By leveraging Bayesian Optimization, the method efficiently balances the trade-off between minimizing flight energy and improving communication for task offloading, providing a robust solution to a complex, multi-objective optimization problem.



