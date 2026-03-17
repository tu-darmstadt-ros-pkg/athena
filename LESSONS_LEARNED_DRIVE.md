# Lessons Learned - Drive

## Initial Version

Due to time constraints, we, at first, tried 3D-printing track profiles from PETG.
We expected low friction, but the results were even worse than expected.
Even a 10° inclination on plywood is impossible to climb with such profiles.

The first actual version was made from PU with a shore hardness of A70.
We used [Poly 75-70, Polyurethan Shore A70](https://syntecshop.com/de/poly-75-70-polyurethan-shore-a70) and
[PT Flex 70](https://syntecshop.com/de/schnellhartendes-polyurethan-guss-gummi-shore-a-70-pt-flex-70).
They should have the same hardness, but the latter hardens more quickly than the former.
During use, unfortunately, the PT Flex profiles broke much more quickly and often than the Poly 75-70 profiles.
The profiles are made from a 3D printed skeleton for structural strength and an outer shell of PU for grip.

## 2026-03-16

Our new version replaces the 3D-printed skeleton with a steel plate to secure the screws, as the previous revision demonstrated that the profiles were ripping out of the screw holes due to the PETG not being robust enough.
The profiles are now fully PU with only a laser-cut 1mm steel plate to ensure they stay on the belt.
Additionally, we joined the two adjacent flipper profiles into a single part, which is connected at the first few mm, to limit bending when a step lands between the two profiles, improving the ability to climb steps.
The height of the flipper profiles was slightly reduced to ensure they are not in contact with the ground when not traversing unstructured terrain or climbing steps.
This improves the robot's ability to turn on the spot.

>[!NOTE]
> Do not add color to the PU, as this significantly reduces the material's resistance to wear and tear.
