#!/bin/sh

echo "What algorithm type are you using? (1:aes, 2:aead, 3:hash, 4:auth/prf or 5:sha256)"

read type

if [ $type == 1 ]
then
	echo "What do you want to use? (1:Software or 2:Hardware)"
	read softhard
	if [ $softhard != 1 ] && [ $softhard != 2 ]
	then
		echo "Not a valid answer!"
		exit
	fi
	if [ $softhard == 1 ]
	then
		echo "What specific algorithm do you want? (1:ECB, 2:CBC or 3:CTR)"
		read alg
		if [ $alg != 1 ] && [ $alg != 2 ] && [ $alg != 3 ]
		then
			echo "Not a valid algorithm!"
			exit
		fi
	elif [ $softhard == 2 ]
	then
		echo "What specific algorithm do you want? (1:ECB, 2:CBC or 3:GCM)"
		read alg
		if [ $alg != 1 ] && [ $alg != 2 ] && [ $alg != 3 ]
		then
			echo "Not a valid algorithm!"
			exit
		fi
	fi
	if [ $alg == 1 ]
	then
		echo "What key size do you want to use (in bits)? (1:128bits, 2:192bits, 3:256bits)"
		read key
		if [ $key != 1 ] && [ $key != 2 ] && [ $key != 3 ]
		then
			echo "Not a valid size!"
			exit
		fi
		if [ $key == 1 ]
		then
			if [ $softhard == 1 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ecb -DTYPE=aes_software -DKEY=128 || exit
			elif [ $softhard == 2 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ecb -DTYPE=aes_hardware -DKEY=128 || exit
			fi
		elif [ $key == 2 ]
		then
			if [ $softhard == 1 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ecb -DTYPE=aes_software -DKEY=192 || exit
			elif [ $softhard == 2 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ecb -DTYPE=aes_hardware -DKEY=192 || exit
			fi
		elif [ $key == 3 ]
		then
			if [ $softhard == 1 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ecb -DTYPE=aes_software -DKEY=256 || exit
			elif [ $softhard == 2 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ecb -DTYPE=aes_hardware -DKEY=256 || exit
			fi
		fi

	elif [ $alg == 2 ]
	then
		echo "What key size do you want to use (in bits)? (1:128bits, 2:192bits, 3:256bits)"
		read key
		if [ $key != 1 ] && [ $key != 2 ] && [ $key != 3 ]
		then
			echo "Not a valid size!"
			exit
		fi
		if [ $key == 1 ]
		then
			if [ $softhard == 1 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=cbc -DTYPE=aes_software -DKEY=128 || exit
			elif [ $softhard == 2 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=cbc -DTYPE=aes_hardware -DKEY=128 || exit
			fi
		elif [ $key == 2 ]
		then
			if [ $softhard == 1 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=cbc -DTYPE=aes_software -DKEY=192 || exit
			elif [ $softhard == 2 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=cbc -DTYPE=aes_hardware -DKEY=192 || exit
			fi
		elif [ $key == 3 ]
		then
			if [ $softhard == 1 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=cbc -DTYPE=aes_software -DKEY=256 || exit
			elif [ $softhard == 2 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=cbc -DTYPE=aes_hardware -DKEY=256 || exit
			fi
		fi

	elif [ $alg == 3 ]
	then
		echo "What key size do you want to use (in bits)? (1:128bits, 2:192bits, 3:256bits)"
		read key
		if [ $key != 1 ] && [ $key != 2 ] && [ $key != 3 ]
		then
			echo "Not a valid size!"
			exit
		fi
		if [ $key == 1 ]
		then
			if [ $softhard == 1 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ctr -DTYPE=aes_software -DKEY=128 || exit
			elif [ $softhard == 2 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=gcm -DTYPE=aes_hardware -DKEY=128 || exit
			fi
		elif [ $key == 2 ]
		then
			if [ $softhard == 1 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ctr -DTYPE=aes_software -DKEY=192 || exit
			elif [ $softhard == 2 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=gcm -DTYPE=aes_hardware -DKEY=192 || exit
			fi
		elif [ $key == 3 ]
		then
			if [ $softhard == 1 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ctr -DTYPE=aes_software -DKEY=256 || exit
			elif [ $softhard == 2 ]
			then
				sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=gcm -DTYPE=aes_hardware -DKEY=256 || exit
			fi

		fi

	fi

elif [ $type == 2 ]
then
	echo "What specific algorithm do you want? (1:ascon-80pq, 2:ascon-128 or 3:ascon-128a)"
	read alg
	if [ $alg != 1 ] && [ $alg != 2 ] && [ $alg != 3 ]
	then
		echo "Not a valid algorithm!"
		exit
	fi

	if [ $alg == 1 ]
	then
		sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ascon-80pq -DTYPE=aead || exit
	elif [ $alg == 2 ]
	then
		sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ascon-128 -DTYPE=aead || exit
	elif [ $alg == 3 ]
	then
		sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ascon-128a -DTYPE=aead || exit
	fi


elif [ $type == 3 ]
then
	echo "What specific algorithm do you want? (1:ascon-hash, 2:ascon-hasha, 3:ascon-xof or 4:ascon-xofa)"
	read alg
	if [ $alg != 1 ] && [ $alg != 2 ] && [ $alg != 3 ] && [ $alg != 4 ]
	then
		echo "Not a valid algorithm!"
		exit
	fi

	if [ $alg == 1 ]
	then
		sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ascon-hash -DTYPE=hash || exit
	elif [ $alg == 2 ]
	then
		sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ascon-hasha -DTYPE=hash || exit
	elif [ $alg == 3 ]
	then
		sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ascon-xof -DTYPE=hash || exit
	elif [ $alg == 4 ]
	then
		sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ascon-xofa -DTYPE=hash || exit
	fi

elif [ $type == 4 ]
then
	echo "What specific algorithm do you want? (1:ascon-mac, 2:ascon-prf or 3:ascon-prfs)"
	read alg
	if [ $alg != 1 ] && [ $alg != 2 ] && [ $alg != 3 ]
	then
		echo "Not a valid algorithm!"
		exit
	fi

	if [ $alg == 1 ]
	then
		sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ascon-mac -DTYPE=prf || exit
	elif [ $alg == 2 ]
	then
		sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ascon-prf -DTYPE=prf || exit
	elif [ $alg == 3 ]
	then
		sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DALG=ascon-prfs -DTYPE=prf || exit
	fi

elif [ $type == 5 ]
then
	sudo cmake -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain .. -DTYPE=sha256 || exit

else
	echo "Not a valid type!"
	exit
fi

sudo make || exit

gcc -o serialprog ../src/serialmain.c

kflash -p /dev/ttyUSB1 kendryte-freertos-project.bin || exit

sudo chmod +x serialprog

./serialprog
