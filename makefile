export ROOT_DIR=${PWD}
# All Target
all: 
	ssh 192.168.1.55 mkdir -p /home/utnso/Documentos/tp-2022-1c-Club-Penguin/CPU/
	rsync -a ${ROOT_DIR}/CPU/ 192.168.1.55:/home/utnso/Documentos/tp-2022-1c-Club-Penguin/CPU/
	ssh -C 192.168.1.55 "cd /home/utnso/Documentos/tp-2022-1c-Club-Penguin/CPU/Debug/ && make"
	ssh -C 192.168.1.55 "dos2unix /home/utnso/Documentos/tp-2022-1c-Club-Penguin/CPU/CPU.config"


	ssh 192.168.1.55 mkdir -p /home/utnso/Documentos/tp-2022-1c-Club-Penguin/Kernel/
	rsync -a ${ROOT_DIR}/Kernel/ 192.168.1.55:/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Kernel/
	ssh -C 192.168.1.55 "cd /home/utnso/Documentos/tp-2022-1c-Club-Penguin/Kernel/Debug/ && make"
	ssh -C 192.168.1.55 "dos2unix /home/utnso/Documentos/tp-2022-1c-Club-Penguin/Kernel/kernel.config"

	ssh 192.168.1.55 mkdir -p /home/utnso/Documentos/tp-2022-1c-Club-Penguin/consola/
	rsync -a ${ROOT_DIR}/consola/ 192.168.1.55:/home/utnso/Documentos/tp-2022-1c-Club-Penguin/consola/
	ssh -C 192.168.1.55 "cd /home/utnso/Documentos/tp-2022-1c-Club-Penguin/consola/Debug/ && make"
	ssh -C 192.168.1.55 "dos2unix /home/utnso/Documentos/tp-2022-1c-Club-Penguin/consola/consola.config"

	ssh 192.168.1.55 mkdir -p /home/utnso/Documentos/tp-2022-1c-Club-Penguin/Memoria/
	rsync -a ${ROOT_DIR}/Memoria/ 192.168.1.55:/home/utnso/Documentos/tp-2022-1c-Club-Penguin/Memoria/
	ssh -C 192.168.1.55 "cd /home/utnso/Documentos/tp-2022-1c-Club-Penguin/Memoria/Debug/ && make"
	ssh -C 192.168.1.55 "dos2unix /home/utnso/Documentos/tp-2022-1c-Club-Penguin/Memoria/memoria.config"

clean:
	CPU
.PHONY: all clean dependents
