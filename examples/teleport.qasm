OPENQASM 2.0;
include "qelib1.inc";
qreg q[3];
creg c[3];

// Bell pair q[1], q[2]
h q[1];
cx q[1],q[2];

// State to teleport on q[0]
h q[0];

cx q[0],q[1];
h q[0];

measure q[0] -> c[0];
measure q[1] -> c[1];

if(c[1]==1) x q[2];
if(c[0]==1) z q[2];

h q[2];
measure q[2] -> c[2];
