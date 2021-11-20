mpirun -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark bcast > out/bcast_2.txt
mpirun -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark gather > out/gather_2.txt
mpirun -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark scatter > out/scatter_2.txt
mpirun -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark reduce > out/reduce_2.txt
mpirun -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark allreduce > out/allreduce_2.txt
mpirun -np 2 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark scan > out/scan_2.txt

mpirun -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark bcast > out/bcast_4.txt
mpirun -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark gather > out/gather_4.txt
mpirun -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark scatter > out/scatter_4.txt
mpirun -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark reduce > out/reduce_4.txt
mpirun -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark allreduce > out/allreduce_4.txt
mpirun -np 4 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark scan > out/scan_4.txt

mpirun -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark bcast > out/bcast_8.txt
mpirun -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark gather > out/gather_8.txt
mpirun -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark scatter > out/scatter_8.txt
mpirun -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark reduce > out/reduce_8.txt
mpirun -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark allreduce > out/allreduce_8.txt
mpirun -np 8 -H master,worker1,worker2,worker3,worker4,worker5,worker6,worker7 ./mpi_benchmark scan > out/scan_8.txt
