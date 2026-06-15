# BBC disk-image packaging for examples.
#
# This file is included from examples/Makefile and uses the shared variables
# defined there: BIN_DIR, BUILD_DIR, DISK_IMAGE_DIR, NETWORK_EXAMPLES,
# CLOCK_EXAMPLES, DISK_EXAMPLES.

# BBC SSD creation helper bundled in this repo.
CREATE_SSD ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))../scripts/create_ssd.py)

# BBC DFS leaf names (7-char limit, explicit to keep them stable)
DFS_NAME_http_get := HTTPGET
DFS_NAME_tcp_get := TCPGET
DFS_NAME_tcp_stream := TCPSTRM
DFS_NAME_clock_test := CLOCK

define GET_DFS_NAME
$(DFS_NAME_$(1))
endef

ifneq ($(strip $(NETWORK_EXAMPLES)),)
DISK_ARTIFACTS += $(DISK_IMAGE_DIR)/network.ssd
endif
ifneq ($(strip $(CLOCK_EXAMPLES)),)
DISK_ARTIFACTS += $(DISK_IMAGE_DIR)/clock.ssd
endif
ifneq ($(strip $(DISK_EXAMPLES)),)
DISK_ARTIFACTS += $(DISK_IMAGE_DIR)/disk.ssd
endif

$(DISK_IMAGE_DIR)/network.ssd: $(foreach ex,$(NETWORK_EXAMPLES),$(BIN_DIR)/$(ex)) | $(DISK_IMAGE_DIR) $(BUILD_DIR)/diskimg
	@echo "Packaging $@..."
	rm -rf "$(BUILD_DIR)/diskimg/network"
	mkdir -p "$(BUILD_DIR)/diskimg/network"
	$(foreach ex,$(NETWORK_EXAMPLES),cp "$(BIN_DIR)/$(ex)" "$(BUILD_DIR)/diskimg/network/$(call GET_DFS_NAME,$(ex))";)
	$(foreach ex,$(NETWORK_EXAMPLES),printf '$.%s 001900 001900\n' "$(call GET_DFS_NAME,$(ex))" > "$(BUILD_DIR)/diskimg/network/$(call GET_DFS_NAME,$(ex)).inf";)
	python3 "$(CREATE_SSD)" -i "$(BUILD_DIR)/diskimg/network" -o "$@" -t NETWORK

$(DISK_IMAGE_DIR)/clock.ssd: $(foreach ex,$(CLOCK_EXAMPLES),$(BIN_DIR)/$(ex)) | $(DISK_IMAGE_DIR) $(BUILD_DIR)/diskimg
	@echo "Packaging $@..."
	rm -rf "$(BUILD_DIR)/diskimg/clock"
	mkdir -p "$(BUILD_DIR)/diskimg/clock"
	$(foreach ex,$(CLOCK_EXAMPLES),cp "$(BIN_DIR)/$(ex)" "$(BUILD_DIR)/diskimg/clock/$(call GET_DFS_NAME,$(ex))";)
	$(foreach ex,$(CLOCK_EXAMPLES),printf '$.%s 001900 001900\n' "$(call GET_DFS_NAME,$(ex))" > "$(BUILD_DIR)/diskimg/clock/$(call GET_DFS_NAME,$(ex)).inf";)
	python3 "$(CREATE_SSD)" -i "$(BUILD_DIR)/diskimg/clock" -o "$@" -t CLOCK

$(DISK_IMAGE_DIR)/disk.ssd: $(foreach ex,$(DISK_EXAMPLES),$(BIN_DIR)/$(ex)) | $(DISK_IMAGE_DIR) $(BUILD_DIR)/diskimg
	@echo "Packaging $@..."
	rm -rf "$(BUILD_DIR)/diskimg/disk"
	mkdir -p "$(BUILD_DIR)/diskimg/disk"
	$(foreach ex,$(DISK_EXAMPLES),cp "$(BIN_DIR)/$(ex)" "$(BUILD_DIR)/diskimg/disk/$(call GET_DFS_NAME,$(ex))";)
	$(foreach ex,$(DISK_EXAMPLES),printf '$.%s 001900 001900\n' "$(call GET_DFS_NAME,$(ex))" > "$(BUILD_DIR)/diskimg/disk/$(call GET_DFS_NAME,$(ex)).inf";)
	python3 "$(CREATE_SSD)" -i "$(BUILD_DIR)/diskimg/disk" -o "$@" -t DISK
