DIRS=budget_pacing bidding_exploration cxr_stats pb_parser data_model ftrl_model
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
