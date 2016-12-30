#make XXX module=dn,control,sn,qp,ors,adapter,qp
DEST=install clean 

ifdef module
CMODULE=${module}
else
CMODULE=all
endif

ifeq ($(CMODULE),all)
	DIRS=protocol util src
endif

ifeq ($(CMODULE),dn)
	DIRS=src/datanode/dn
endif
ifeq ($(CMODULE),control)
	DIRS=src/control
endif
ifeq ($(CMODULE),sn)
	DIRS=src/sn
endif
ifeq ($(CMODULE),qp)
	DIRS=src/poseidon_qp
endif
ifeq ($(CMODULE),ors)
	DIRS=src/ors
endif
ifeq ($(CMODULE),adapter)
	DIRS=src/adapter
endif


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

