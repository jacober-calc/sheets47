# Convenience entry points; tools/build.py is the build implementation.
.PHONY: host test device viewer
host test:
	python3 -B tools/build.py host
device:
	python3 -B tools/build.py device
viewer:
	python3 -B tools/host_viewer.py
# No clean target: deletion requires explicit user approval for exact paths.
