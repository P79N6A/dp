DIRS=comm_event monitor zk4cpp ha ors_offline control sn  log_server  ors   adapter poseidon_qp mem_sync #datanode/dn
DEST=install clean 

all:%: 
	@for x in $(DIRS); \
	do \
	make -C $$x $@; \
	if [ $$? != 0 ]; then \
		exit 1; \
	fi \
	done
	@echo "build successfully!"

${DEST}:%:
	@for x in $(DIRS); \
	do \
		make -C $$x $@; \
	done
