.PHONY : all test coverage phase1 phase2 phase3 phase4 phase5 phase6 phase7

all: phase1 phase2 phase3 phase4 phase5 phase6 phase7

test:
	$(MAKE) -C test test

coverage:
	$(MAKE) -C test coverage

LOG_COLOR = \033[0;33m
LOG_NOCOLOR = \033[0m

phase1:
	@printf "$(LOG_COLOR)### Build and run $@ ###$(LOG_NOCOLOR)\n\n"
	$(MAKE) -C phase_1 rebuild
	$(MAKE) -C phase_1 run
	@printf "$(LOG_COLOR)### End of $@ ###$(LOG_NOCOLOR)\n\n"

phase2:
	@printf "$(LOG_COLOR)### Build and run $@ ###$(LOG_NOCOLOR)\n\n"
	$(MAKE) -C phase_2 rebuild
	$(MAKE) -C phase_2 run
	@printf "$(LOG_COLOR)### End of $@ ###$(LOG_NOCOLOR)\n\n"

phase3:
	@printf "$(LOG_COLOR)### Build and run $@ ###$(LOG_NOCOLOR)\n\n"
	$(MAKE) -C phase_3 rebuild
	$(MAKE) -C phase_3 run
	@printf "$(LOG_COLOR)### End of $@ ###$(LOG_NOCOLOR)\n\n"

phase4:
	@printf "$(LOG_COLOR)### Build and run $@ ###$(LOG_NOCOLOR)\n\n"
	$(MAKE) -C phase_4 rebuild
	$(MAKE) -C phase_4 run
	@printf "$(LOG_COLOR)### End of $@ ###$(LOG_NOCOLOR)\n\n"

phase5:
	@printf "$(LOG_COLOR)### Build and run $@ ###$(LOG_NOCOLOR)\n\n"
	$(MAKE) -C phase_5 rebuild
	$(MAKE) -C phase_5 run
	@printf "$(LOG_COLOR)### End of $@ ###$(LOG_NOCOLOR)\n\n"

phase6:
	@printf "$(LOG_COLOR)### Build and run $@ ###$(LOG_NOCOLOR)\n\n"
	$(MAKE) -C phase_6 rebuild
	$(MAKE) -C phase_6 run
	@printf "$(LOG_COLOR)### End of $@ ###$(LOG_NOCOLOR)\n\n"

phase7:
	@printf "$(LOG_COLOR)### Build and run $@ ###$(LOG_NOCOLOR)\n\n"
	$(MAKE) -C phase_7 build
	$(MAKE) -C phase_7 run
	@printf "$(LOG_COLOR)### End of $@ ###$(LOG_NOCOLOR)\n\n"
