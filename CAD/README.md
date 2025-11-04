## ATHENA CAD design

This directory contains the CAD files for the Rescue Robot ATHENA. The accompanying documentation outlines each component, its material specification, and the recommended manufacturing process.

The provided CAD model is compatible with Autodesk Fusion 360, and all components are integrated within a single comprehensive assembly file.

**Important** Before importing the model into Fusion 360, ensure that a dedicated folder is created for the project files.

## CAD Description

The primary file, main_assembly, represents the complete mechanical assembly of ATHENA. It includes all sub-assemblies and defines their kinematic and structural relationships.

Both the Flipper and Arm sub-assemblies are fully articulated within the model. They can be repositioned interactively and will automatically return to their defined resting configurations.

ATHENA consists of the following major sub-assemblies:

- sub_assembly_chassis: Composed of bonded and fastened carbon fiber composite (CFK) panels forming the primary load-bearing chassis of the robot.

- four sub_assembly_tracked_drive: Four identical tracked drive units, each integrating a drive motor, drive pulley, and flipper sub-assembly.

- two sub_assembly_battery_box: Two identical removable battery modules, each designed to accommodate a pair of 4S LiPo batteries for modular power management.

- assembly_arm_7_DoF: A 7-DoF manipulator assembly. Each joint includes predefined motion constraints, allowing spatial motion simulation and evaluation of various operational configurations.

## ATHENA Parts, Materials & Manufacturing Specification

This section lists all mechanical components of ATHENA, organized by sub-assembly. Each table specifies the part name, material, recommended manufacturing process, and additional notes.

---

## 1. Sub-Assembly: Chassis

| Part Name / ID | Material | Manufacturing Process | Notes / Remarks |
|----------------|----------|---------------------|----------------|
| base_plate     | 7075 Aluminium | Laser/Plasma cutting |    -            |
| front_plate x2 | CFK      | CNC mill/ waterjet cutting | load bearing part, fibre arrangement should be considered |
| top_plate      | CFK      | CNC mill/ waterjet cutting | load bearing part, fibre arrangement should be considered |
| side_plate     | CFK      | CNC mill/ waterjet cutting | load bearing part, fibre arrangement should be considered |
| main_track_holder| CFK      | CNC mill/ waterjet cutting | load bearing part, fibre arrangement should be considered |
| CFK_angles x12 | CFK      | COTS                | cut in required lengths |
| battery_box_rails| PETG | 3D printed            | -               |
| Flipper_motor_holder| Stainless Steel 316L| sheet metal bending |     -           |
| ball_bearin_flange_20x32x7 x4| Steel | COTS     | -               | 
| main_track_holder_cover | GFK and PETG | Hand laminated / 3D printed| -               |
| U_profile      | Aluminium| COTS                | -               |
| M3_nut_holder  | PETG     | 3D printed          | -               |
| Electronics holders | PETG| 3D printed          | -               |
| Carry_Reinforcement x4 | Steel | Laser cut      | -               |
| battery_box_end_plate x2 | PETG | 3D printed    | -               |
| sensor holders | PETG     | 3D printed          | -               |
| flipper_motors x4 |  -     | COTS                | Part number: H54p-200_M54P-060 |

> **Notes:** The chassis is primarily composed of CFK components bonded and fastened to form the load-bearing frame. Maintain tight tolerances and ensure adhesive joints are properly cured. COTS: Commercial Off-The-Shelf

---

## 2. Sub-Assembly: Tracked Drive (x4)

| Part Name / ID | Material | Manufacturing Process | Notes / Remarks |
|----------------|----------|---------------------|----------------|
| Unitree A1 motor |          | COTS                | [Link](https://www.unitree.com/a1/motor) |
| motor_axle_connector | Stainless Steel| CNC machined      | Use high strength Loctite when mounting    |
| drive_shaft    | Steel    | CNC machining (COTS)| Part number: KZAC20-180-NA6-NB6-TA63-KA8-HA30-KB86-HB30-KC120-HC60  [Link](https://de.misumi-ec.com/vona2/detail/110300098640/?HissuCode=KZAC20-180-NA6-NB6-KA8-HA30-KB86-HB30-KC120-HC60-TA63&PNSearch=KZAC20-180-NA6-NB6-KA8-HA30-KB86-HB30-KC120-HC60-TA63&KWSearch=KZAC20-180-NA6-NB6-TA63-KA8-HA30-KB86-HB30-KC120-HC60&searchFlow=results2products&list=PageSearchResult) |
| shaft _keyway_KEG6-30| Steel    | COTS                | Part number: KEG6-60  [Link](https://de.misumi-ec.com/vona2/detail/110300253770/?HissuCode=KEG6-60&PNSearch=KEG6-60&KWSearch=KEG6-60&searchFlow=results2products&list=PageSearchResult) |
| shaft_keyway_KES6-30 x2| Steel | COTS             | Part number: KES6-30 [Link](https://de.misumi-ec.com/vona2/detail/110300253770/?HissuCode=KES6-30&PNSearch=KES6-30&KWSearch=KES6-30&searchFlow=results2products&list=PageSearchResult) |
| drive_wheel    | PETG     | 3D printed          | minimun 6 parameters while printing to maintain rigid structure |
| WheelSpacer    | PETG     | 3D printed          | -               |
| FlipperSpacer  | PETG     | 3D printed          | -              |
| ballbearing_20x42x16 | Steel         | COTS     | -               |
| toothed_belt_pulley_48_flipper | Aluminium 7075 | CNC machining  | -               |
| geared_pulley_24 | Aluminium 7075 | CNC Machining | -               |
| flipper_base_plate_inside | CFK       | CNC machining | load bearing part, correct fibre arrangement should be considered |
| flipper_base_plate_outside| CFK       | CNC machining | load bearing part, correct fibre arrangement should be considered |
| flippersicherung | Aluminium | Lasercut         |                |
|  shaft_collar_SCCJ20-6 | Steel | COTS           | Part number: SCCJ20-6 [Link](https://de.misumi-ec.com/vona2/detail/110302636320/?HissuCode=SCCJ20-6&PNSearch=SCCJ20-6&KWSearch=SCCJ20-6&searchFlow=results2products&list=PageSearchResult)|
| belt_pulley_bottom | PETG and Stainless Steel 316L | 3d printed and laser cut | two laser cut steel plates screwed and sandwiched between three 3D printed parts |
| belt_pulley_top | PETG    | 3D printed          | minimun 6 parameters while printing to maintain rigid structure               |
| Flipperbelt    | PU and Steel | COTS            |                |
| spacer_ARU-340 x8 | Stainless Steel | COTS         | Part number: ARU-340 [Link](https://de.misumi-ec.com/vona2/detail/221006202465/?HissuCode=ARU-340&PNSearch=ARU-340&KWSearch=ARU-340&searchFlow=results2products&list=PageSearchResult) |
| flipper_small_wheel x5| PETG | 3D printed       | -              |
| ballbearing_6x17x6 x10 | Steel | COTS           | for flipper small wheel |
| ballbaering_flange_20x32x7 | Steel | COTS       | -              |
| ballbearing_flange_8x14x4 x2 | Steel | COTS     | -              |
| wheel spacers x3 | PETG   | 3D printed          | -              |
| shaft_SFHRW8-49-M6-N6 | Steel | COTS            | Part number: SFHRW8-49-M6-N6 [Link](https://de.misumi-ec.com/vona2/detail/110300089020/?HissuCode=SFHRW8-49-M6-N6&PNSearch=SFHRW8-49-M6-N6&KWSearch=SFHRW8-49-M6-N6&searchFlow=results2products&list=PageSearchResult) |

> **Notes:** Each tracked drive unit is identical and integrates a drive motor, drive pulley, and flipper linkage. The flipper assembly interfaces directly with the tracked drive unit. Components must be lightweight yet rigid to withstand dynamic loads.

---

## 3. Sub-Assembly: Battery Box (x2)

| Part Name / ID | Material | Manufacturing Process | Notes / Remarks |
|----------------|----------|---------------------|----------------|
| battery_box    | PETG     | 3D printed          | support structure neccessary wile printing |
| battery_box_front_plate | CFK | CNC machining/waterjet cutting |                |
| latch_C-10-2 x2| Brass    | COTS                | Part number: C-10-2 [Link](https://de.misumi-ec.com/vona2/detail/221004914126/?HissuCode=C-10-2&PNSearch=C-10-2&KWSearch=C-10-2&searchFlow=results2products&list=PageSearchResult) |
| battery x2     | -        | COTS                | Recommended: LiPo 4S, 25C, 6750 mAh |
| XT60E-F x2     | Plastic/Copper | COTS          | may be replaced in fure iteration |

> **Notes:** Each battery box houses two 4S LiPo batteries and is designed for quick replacement. Use heat-resistant materials and ensure electrical insulation integrity.

---

## 4. Sub-Assembly: Arm (7-DoF Manipulator)

| Part Name / ID | Material | Manufacturing Process | Notes / Remarks |
|----------------|----------|---------------------|----------------|
|                |          |                     |                |
|                |          |                     |                |
|                |          |                     |                |

> **Notes:** Each joint includes predefined angular limits and housing components. Consider mass balancing and precision machining for smooth motion and accuracy.

---

## 5. Global / Common Components


