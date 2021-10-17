build-lambdaml:
	cp memcache.py "$(ARTIFACTS_DIR)/"
	cp -r communicator "$(ARTIFACTS_DIR)/"
	cp -r data_loader "$(ARTIFACTS_DIR)/"
	cp -r model "$(ARTIFACTS_DIR)/"
	cp -r storage "$(ARTIFACTS_DIR)/"
	cp -r utils "$(ARTIFACTS_DIR)/"
	cp -r examples "$(ARTIFACTS_DIR)/"

build-lambdaMLDeps:
	pip install redis -t "$(ARTIFACTS_DIR)/python"
	cp -r extern/SMI/* "$(ARTIFACTS_DIR)/python/"