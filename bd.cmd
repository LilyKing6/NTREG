@md build
@cd build
@cmake -G "Unix Makefiles" ..
@make -j 8
@echo Build completed successfully!
@pause
@cd ..