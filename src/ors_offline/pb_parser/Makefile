DIRS=budget_pacing_model bidding_exploration_model youku_content_grade cxr_stats
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
