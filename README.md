# Athena

<img src="https://github.com/user-attachments/assets/25306f4f-53d6-490b-b317-e6f872345314" alt="Picture of the Athena robot with four reconfigurable flippers and a 7-degree-of-freedom manipulator arm inspecting an object." width="800"/>

> [!note]
> This repository is still being filled.
> Lessons learned will be added soon.
> New data will be followed by a new release, so you can just watch for releases.

This repository documents the Athena rescue robot.
All required CAD and KiCAD files are available here for your reference and inspiration.
Copyright remains with the individual contributors, and commercial use is not permitted.

Athena was designed as a research robot for student projects on obstacle traversal and manipulation in rough terrain.
A whole-body crawler design enables the robot to traverse unstructured grounds without the risk of getting stuck, and
the four reconfigurable flippers enable Athena to adapt to the terrain and overcome steps of up to 41cm.
The large manipulator has a reach of up to 1.54 meters and can lift up to 2.9 kilograms at full extension and 7.2
kilograms at 0.5 meters.
With 7 degrees of freedom and 4 continuous joints with slip rings, it is extremely versatile and well-suited for
autonomous manipulation.

The environment perception modules consisting of a Livox MID-360, an Orbbec Astra Stereo S U 3 RGBD camera, and a 240°
FOV wide-angle camera enable comprehensive colored 3D environment perception.

<img src="https://github.com/user-attachments/assets/ad7d781e-b4f8-4ccd-bbb2-22abf1279ee5" height="360"/>

<img height="360" alt="The FOV of the individual sensors of the environment perception module" src="https://github.com/user-attachments/assets/75c30095-1a4d-4143-95c6-520780d4ad9e" />


## CAD

The [CAD](CAD) folder contains all CAD data as Fusion archive for editing and alternatively also as STEP.
Please note that the belts are mostly visual only, and the actual belt lengths are not reflected in the model.

## Electronics

In the [electronics](electronics) folder, the custom PCBs used to distribute power and ensure safety during operation can be found.

## Remote E-Stop

The source code and bill of materials for the ~80€ remote e-stop can be found in [E-Stop](E-Stop).
This ESP32-based remote E-Stop transmits its state over BLE, ESP-NOW and LoRa for high reliability and low triggering times of up (or down) to 10ms.

## Stats

| | |
|-|-|
| Max Speed             | 1.5m/s |
| Max Step Height       | 41cm   |
| Max manipulator reach | 1.54m  |
| Max payload @0.5m     | 7.2kg  |
| Max payload @1m       | 4.8kg  |
| Max payload @1.54m    | 2.9kg  |

## Software

* **Motor Driver**: [athena_motor_driver](https://github.com/tu-darmstadt-ros-pkg/athena_motor_driver) provides firmware for the Teensy and ROS driver to torque-control the Unitree A1 motors. 

## Pictures

<img src="https://github.com/user-attachments/assets/71a8bbf7-2d84-4618-9528-5a618330b165" alt="The robot Athena on the maximum step height that was tested at 41cm" width="800"/>
<br/>
<img src="https://github.com/user-attachments/assets/bfc532a7-3277-4dac-a2e8-db4569d3366b" alt="The robot Athena on 45 degree industrial metal stairs." width="402"/>

<img src="https://github.com/user-attachments/assets/0aa31b29-0e72-4540-bcff-a4a3ee0761b0" alt="The robot Athena lifting a gallon and multiple water bottles totaling about 7.2kg" width="392"/>
