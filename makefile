BUILDDIR := build

disocamera: $(BUILDDIR)/cam.o $(BUILDDIR)/main.o
	@echo "\033[92m>\033[0m"
	@echo "\033[92m>\033[0m"
	@echo "\033[92m>\033[0m"
	@echo "\033[92mBuilding executable...\033[0m"
	g++ -std=c++17 -o disocamera $(BUILDDIR)/main.o $(BUILDDIR)/cam.o
	@echo "\033[92m... done !\033[0m"
	@echo "\034[92m>\034[0m"
	@echo "\034[92m>\034[0m"
	@echo "\034[92m>\034[0m"
	@echo "\034[92m> All done !\034[0m"

$(BUILDDIR)/main.o: main.cpp
	@echo "\033[92m>\033[0m"
	@echo "\033[92m>\033[0m"
	@echo "\033[92m>\033[0m"
	@echo "\033[92mCompiling main ...\033[0m"
	cd $(BUILDDIR) && g++ -std=c++17 -c ../main.cpp
	@echo "\033[92m... done !\033[0m"

$(BUILDDIR)/cam.o: cam.cpp
	@echo "\033[92m>\033[0m"
	@echo "\033[92m>\033[0m"
	@echo "\033[92m>\033[0m"
	@echo "\033[92m> Compiling cam ...\033[0m"
	cd $(BUILDDIR) && g++ -std=c++17 -c ../cam.cpp
	@echo "\033[92m... done !\033[0m"

clean:
	rm -rf build/*
	@echo "\033[92m>>>\033[0m"
	@echo "\033[92m.+* Clean *+.\033[0m"
	@echo "\033[92m>>>\033[0m"