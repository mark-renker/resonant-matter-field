declare name "Resonant Matter Field — Faust Core";
declare author "Resonant Matter Lab";
declare description "Reduced spatial microcontact / electromechanical modal synthesis core";
declare license "MIT";

import("stdfaust.lib");

freq       = hslider("MIDI/freq[unit:Hz]", 110, 20, 2000, 0.01);
gate       = button("MIDI/gate");
gain       = hslider("MIDI/gain", 0.8, 0, 1, 0.001);
density    = hslider("Contact/density[unit:/s]", 18, 0.2, 220, 0.1);
duration   = hslider("Contact/duration[unit:ms]", 18, 1, 180, 0.1);
force      = hslider("Contact/force", 0.45, 0, 1, 0.001);
roughness  = hslider("Contact/roughness", 0.35, 0, 1, 0.001);
x          = hslider("Contact/x", 0.37, 0.02, 0.98, 0.001);
y          = hslider("Contact/y", 0.61, 0.02, 0.98, 0.001);
stiffness  = hslider("Body/stiffness", 1, 0.25, 4, 0.001);
decay      = hslider("Body/decay[unit:s]", 1.8, 0.05, 12, 0.001);
coupling   = hslider("Circuit/BL_coupling", 0.35, 0, 1, 0.001);
loading    = hslider("Circuit/loading", 0.42, 0, 1, 0.001);
activeZ    = hslider("Circuit/active_impedance", 0, -0.8, 0.8, 0.001);
width      = hslider("Radiation/width", 0.7, 0, 1, 0.001);
outDB      = hslider("Output/level[unit:dB]", -9, -36, 6, 0.1);

period     = max(1, ma.SR / max(0.2, density));
contacts   = ba.pulse(period) * gate;
envelope   = contacts : si.smoo;
surface    = (1-roughness) + roughness * no.noise;
excitation = envelope * surface * force * gain;
effectiveDecay = decay * max(0.12, 1 + 2.2*activeZ - 0.6*loading*coupling);
qFor(f) = max(2, 3.14159265 * f * effectiveDecay);

shape(nx,ny,px,py) = sin(3.14159265*nx*px) * sin(3.14159265*ny*py);
res(r,nx,ny) = excitation * shape(nx,ny,x,y) : fi.resonbp(freq*stiffness*r, qFor(freq*stiffness*r), 0.08);

// Built-in 13-EDT Bohlen–Pierce lattice. The production JUCE wrapper loads
// arbitrary-period Scala files at runtime and rebuilds the corresponding table.
m1  = res(1.000000,1,1); m2  = res(1.088183,2,1); m3  = res(1.184132,1,2);
m4  = res(1.288547,2,2); m5  = res(1.402151,3,1); m6  = res(1.525803,1,3);
m7  = res(1.660359,3,2); m8  = res(1.806782,2,3); m9  = res(1.966116,4,1);
m10 = res(2.139463,1,4); m11 = res(2.328070,3,3); m12 = res(2.533303,4,2);

pickL(nx,ny) = shape(nx,ny,0.5-0.38*width,0.31);
pickR(nx,ny) = shape(nx,ny,0.5+0.38*width,0.69);
left  = m1*pickL(1,1)+m2*pickL(2,1)+m3*pickL(1,2)+m4*pickL(2,2)+m5*pickL(3,1)+m6*pickL(1,3)
      + m7*pickL(3,2)+m8*pickL(2,3)+m9*pickL(4,1)+m10*pickL(1,4)+m11*pickL(3,3)+m12*pickL(4,2);
right = m1*pickR(1,1)+m2*pickR(2,1)+m3*pickR(1,2)+m4*pickR(2,2)+m5*pickR(3,1)+m6*pickR(1,3)
      + m7*pickR(3,2)+m8*pickR(2,3)+m9*pickR(4,1)+m10*pickR(1,4)+m11*pickR(3,3)+m12*pickR(4,2);

process = left*ba.db2linear(outDB), right*ba.db2linear(outDB);
