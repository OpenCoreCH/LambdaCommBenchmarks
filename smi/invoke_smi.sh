mpirun -x LD_LIBRARY_PATH=. -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark bcast > out/bcast_2.txt
mpirun -x LD_LIBRARY_PATH=. -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark gather > out/gather_2.txt
mpirun -x LD_LIBRARY_PATH=. -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark scatter > out/scatter_2.txt
mpirun -x LD_LIBRARY_PATH=. -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark reduce > out/reduce_2.txt
mpirun -x LD_LIBRARY_PATH=. -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark allreduce > out/allreduce_2.txt
mpirun -x LD_LIBRARY_PATH=. -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark scan > out/scan_2.txt

mpirun -x LD_LIBRARY_PATH=. -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark bcast > out/bcast_4.txt
mpirun -x LD_LIBRARY_PATH=. -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark gather > out/gather_4.txt
mpirun -x LD_LIBRARY_PATH=. -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark scatter > out/scatter_4.txt
mpirun -x LD_LIBRARY_PATH=. -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark reduce > out/reduce_4.txt
mpirun -x LD_LIBRARY_PATH=. -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark allreduce > out/allreduce_4.txt
mpirun -x LD_LIBRARY_PATH=. -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark scan > out/scan_4.txt

mpirun -x LD_LIBRARY_PATH=. -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark bcast > out/bcast_8.txt
mpirun -x LD_LIBRARY_PATH=. -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark gather > out/gather_8.txt
mpirun -x LD_LIBRARY_PATH=. -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark scatter > out/scatter_8.txt
mpirun -x LD_LIBRARY_PATH=. -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark reduce > out/reduce_8.txt
mpirun -x LD_LIBRARY_PATH=. -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark allreduce > out/allreduce_8.txt
mpirun -x LD_LIBRARY_PATH=. -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./smi_benchmark scan > out/scan_8.txt
