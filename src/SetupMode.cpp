#include "SetupMode.h"
#include "Logger.h"

// Constructor
SetupMode::SetupMode() 
    : currentState(SetupState::IDLE), isActive(false),
      mappingStore(nullptr), sdScanner(nullptr), rfidManager(nullptr), buttonManager(nullptr),
      currentPathIndex(0), contentRoot("/test_music"), assignedCount(0), skippedCount(0), totalProcessed(0),
      setupStartTime(0), lastRfidRead(0), lastButtonCheck(0),
      encoderPressed(false), encoderPressStart(0), lastButtonAction(SetupButtonAction::NONE), lastButtonState(BUTTON_NONE) {
}

// Initialize setup mode
bool SetupMode::begin(MappingStore* store, SdScanner* scanner, RFID_Manager* rfid, Button_Manager* buttons, const String& root) {
    if (!store || !scanner || !rfid || !buttons) {
        LOG_SETUP_ERROR("Invalid components provided");
        return false;
    }
    
    mappingStore = store;
    sdScanner = scanner;
    rfidManager = rfid;
    buttonManager = buttons;
    contentRoot = root;
    
    LOG_SETUP_INFO("Initialized with content root: %s", contentRoot.c_str());
    return true;
}

// Load content root from settings
bool SetupMode::loadContentRootFromSettings(const char* settingsKey) {
    // Default content root if settings fail
    contentRoot = "/test_music";
    
    // Try to load from settings (you can integrate with your Settings_Manager here)
    // For now, we'll use a hardcoded default, but this provides the hook for settings integration
    
    LOG_SETUP_INFO("Content root set to: %s", contentRoot.c_str());
    LOG_SETUP_INFO("To customize, implement settings integration or call setContentRoot()");
    
    return true;
}

bool SetupMode::isInitialized() const {
    return mappingStore && sdScanner && rfidManager;
}

// Enter setup mode
void SetupMode::enter() {
    if (!isInitialized()) {
        LOG_SETUP_ERROR("Not initialized, cannot enter");
        return;
    }
    
    LOG_SETUP_INFO("=== ENTERING SETUP MODE ===");
    isActive = true;
    currentState = SetupState::SETUP_INIT;
    setupStartTime = millis();
    currentPathIndex = 0;
    
    // Disable RFID audio control during setup to prevent playback triggers
    rfidManager->enableAudioControl(false);
    LOG_SETUP_INFO("RFID audio control disabled");
    
    // Reset counters
    assignedCount = 0;
    skippedCount = 0;
    totalProcessed = 0;
}

// Exit setup mode
void SetupMode::exit() {
    LOG_SETUP_INFO("=== EXITING SETUP MODE ===");
    isActive = false;
    currentState = SetupState::IDLE;
    
    // Re-enable RFID audio control
    rfidManager->enableAudioControl(true);
}

// Main setup loop
void SetupMode::loop() {
    if (!isActive || !isInitialized()) return;
    
    // Handle button actions
    SetupButtonAction buttonAction = getButtonAction();
    if (buttonAction != SetupButtonAction::NONE) {
        handleButtonAction(buttonAction);
    }
    
    // State machine
    switch (currentState) {
        case SetupState::SETUP_INIT:
            stepInit();
            break;
        case SetupState::PROMPT_PRESENT_TAG:
            stepPrompt();
            break;
        case SetupState::READ_UID:
            stepReadUid();
            break;
        case SetupState::CONFIRM_OVERWRITE:
            stepConfirmOverwrite();
            break;
        case SetupState::WAIT_FOR_CARD_REMOVAL:
            stepWaitForCardRemoval();
            break;
        case SetupState::RFID_SETUP_SUMMARY:
            stepSummary();
            break;
        default:
            break;
    }
}

// Initialize setup
void SetupMode::stepInit() {
    LOG_SETUP_INFO("Initializing setup...");
    
    // Ensure mapping file exists
    if (!mappingStore->isInitialized()) {
        LOG_SETUP_ERROR("Mapping store not initialized");
        exit();
        return;
    }
    
    // Load existing mappings
    if (!mappingStore->loadAll()) {
        LOG_SETUP_ERROR("Failed to load mappings");
        exit();
        return;
    }
    
    // Scan SD directories (non-recursive, one level deep)
    if (!sdScanner->listAudioDirs(SD_MMC, contentRoot, unassignedPaths)) {
        LOG_SETUP_ERROR("Failed to scan SD directories");
        exit();
        return;
    }
    
    // Compute unassigned paths
    computeUnassignedPaths();
    
    if (unassignedPaths.empty()) {
        LOG_SETUP_INFO("No unassigned directories found");
        stepSummary();
        return;
    }
    
    LOG_SETUP_INFO("Found %d unassigned directories", unassignedPaths.size());
    
    // Move to first unassigned directory
    currentState = SetupState::PROMPT_PRESENT_TAG;
}

// Prompt for tag presentation
void SetupMode::stepPrompt() {
    if (currentPathIndex >= unassignedPaths.size()) {
        currentState = SetupState::RFID_SETUP_SUMMARY;
        return;
    }
    
    currentFolder = unassignedPaths[currentPathIndex];
    showPrompt("Present tag for: " + currentFolder + " (Play=confirm, Back=cancel, Next=skip)");
    showStatus("Timeout in 30s - Back to cancel folder, long-press to abort setup");
    
    currentState = SetupState::READ_UID;
    lastRfidRead = millis();
}

// Read RFID UID
void SetupMode::stepReadUid() {
    // Check for timeout
    if (millis() - lastRfidRead > RFID_TIMEOUT) {
        showStatus("Timeout - use Next to skip or Back to cancel");
        return;
    }
    
    // Wait for RFID with debouncing
    String uid;
    if (waitForRfid(uid, 100)) { // 100ms timeout for this check
        handleUid(uid);
    }
}

// Confirm overwrite if UID already mapped
void SetupMode::stepConfirmOverwrite() {
    showPrompt("UID " + currentUid + " already used by another folder - overwrite? (Play=yes, Back=no)");
    
    // Wait for button response
    SetupButtonAction action = getButtonAction();
    if (action == SetupButtonAction::PLAY_OK) {
        // Confirm overwrite
        if (mappingStore->rebind(currentUid, currentFolder)) {
            assignedCount++;
            totalProcessed++;
            showStatus("Assigned to " + currentFolder + " - Remove card and press Play/Pause to continue");
            currentState = SetupState::WAIT_FOR_CARD_REMOVAL;
        } else {
            showStatus("Failed to assign - try again");
            currentState = SetupState::PROMPT_PRESENT_TAG;
        }
    } else if (action == SetupButtonAction::BACK_PREV) {
        // Cancel overwrite
        showStatus("Cancelled - try different tag");
        currentState = SetupState::PROMPT_PRESENT_TAG;
    }
}

// Wait for card removal after assignment
void SetupMode::stepWaitForCardRemoval() {
    // Check if RFID card is still present
    if (!rfidManager->isTagPresent()) {
        // Card removed - advance to next folder
        showStatus("Card removed - advancing to next folder");
        currentPathIndex++;
        currentState = SetupState::PROMPT_PRESENT_TAG;
    } else {
        // Card still present - show instruction
        showPrompt("Remove RFID card and press Play/Pause to continue");
    }
}

// Show summary and exit
void SetupMode::stepSummary() {
    LOG_SETUP_INFO("=== SETUP SUMMARY ===");
    LOG_SETUP_INFO("Directories processed: %d", totalProcessed);
    LOG_SETUP_INFO("Mappings created: %d", assignedCount);
    LOG_SETUP_INFO("Skipped: %d", skippedCount);
    LOG_SETUP_INFO("Already assigned: %d", totalProcessed - assignedCount - skippedCount);
    LOG_SETUP_INFO("=====================");
    
    exit();
}

// Handle UID detection
void SetupMode::handleUid(const String& uid) {
    // Normalize UID immediately for consistency (strips colons, spaces, uppercases)
    currentUid = uid;
    showStatus("Detected UID: " + uid);
    
    // Check if UID is already mapped
    String existingPath;
    if (mappingStore->getPathFor(uid, existingPath)) {
        if (existingPath == currentFolder) {
            // Already mapped to same folder
            showStatus("Already assigned to this folder - Next to continue");
            currentPathIndex++;
            totalProcessed++;
            currentState = SetupState::PROMPT_PRESENT_TAG;
        } else {
            // Mapped to different folder - ask for overwrite
            showStatus("Already used by: " + existingPath + " - overwrite? (Play=yes, Back=no)");
            currentState = SetupState::CONFIRM_OVERWRITE;
        }
    } else {
        // New UID - assign immediately
        Mapping mapping(uid, currentFolder);
        if (mappingStore->append(mapping)) {
            assignedCount++;
            totalProcessed++;
            showStatus("Assigned to " + currentFolder + " - Remove card and press Play/Pause to continue");
            currentState = SetupState::WAIT_FOR_CARD_REMOVAL;
        } else {
            showStatus("Failed to assign - try again");
            currentState = SetupState::PROMPT_PRESENT_TAG;
        }
    }
}

// Handle button actions
void SetupMode::handleButtonAction(SetupButtonAction action) {
    switch (action) {
        case SetupButtonAction::PLAY_OK:
            // Confirm action or advance to next stage
            if (currentState == SetupState::CONFIRM_OVERWRITE) {
                // Handle in stepConfirmOverwrite
                return;
            } else if (currentState == SetupState::WAIT_FOR_CARD_REMOVAL) {
                // Force advance to next folder when play/pause is pressed (even if card still present)
                showStatus("Advancing to next folder (Play/Pause pressed)");
                currentPathIndex++;
                currentState = SetupState::PROMPT_PRESENT_TAG;
            } else if (currentState == SetupState::PROMPT_PRESENT_TAG || 
                       currentState == SetupState::READ_UID) {
                // Advance to next folder when play/pause is pressed
                showStatus("Skipped - Next folder (Play/Pause pressed)");
                currentPathIndex++;
                totalProcessed++;
                currentState = SetupState::PROMPT_PRESENT_TAG;
            }
            break;
            
        case SetupButtonAction::BACK_PREV:
            // Cancel/deny action
            if (currentState == SetupState::CONFIRM_OVERWRITE) {
                // Handle in stepConfirmOverwrite
                return;
            }
            // Exit setup mode
            exit();
            break;
            
        case SetupButtonAction::NEXT:
            // Skip current folder
            if (currentState == SetupState::PROMPT_PRESENT_TAG || 
                currentState == SetupState::READ_UID) {
                skippedCount++;
                totalProcessed++;
                showStatus("Skipped - Next folder");
                currentPathIndex++;
                currentState = SetupState::PROMPT_PRESENT_TAG;
            }
            break;
            
        case SetupButtonAction::LONG_PRESS:
            // Exit setup mode
            exit();
            break;
            
        default:
            break;
    }
}

// Get button action from hardware
SetupButtonAction SetupMode::getButtonAction() {
    if (!buttonManager) return SetupButtonAction::NONE;
    
    // Check for long press on encoder (simplified - integrate with your Rotary_Manager if needed)
    if (encoderPressed) {
        if (millis() - encoderPressStart > LONG_PRESS_THRESHOLD) {
            encoderPressed = false;
            return SetupButtonAction::LONG_PRESS;
        }
    }
    
    // Get current button state from Button_Manager
    ButtonType currentButton = buttonManager->getCurrentButton();
    
    // Check if button state changed
    if (currentButton != lastButtonState) {
        lastButtonState = currentButton;
        lastButtonCheck = millis();
        
        // Map button types to setup actions
        switch (currentButton) {
            case BUTTON_PLAY_PAUSE:
                return SetupButtonAction::PLAY_OK;
            case BUTTON_PREVIOUS:
                return SetupButtonAction::BACK_PREV;
            case BUTTON_NEXT:
                return SetupButtonAction::NEXT;
            default:
                break;
        }
    }
    
    return SetupButtonAction::NONE;
}

// Wait for RFID with debouncing
bool SetupMode::waitForRfid(String& uid, unsigned long timeout) {
    static String lastUid = "";
    static unsigned long lastReadTime = 0;
    static int consecutiveCount = 0;
    static bool tagPresent = false;
    
    // Check if RFID is present
    if (rfidManager->isTagPresent()) {
        String currentUid = rfidManager->getLastDetectedUIDString();
        
        if (currentUid != lastUid) {
            // New UID detected
            lastUid = currentUid;
            lastReadTime = millis();
            consecutiveCount = 1;
            tagPresent = true;
        } else if (millis() - lastReadTime > 200) { // 200ms debounce
            consecutiveCount++;
            lastReadTime = millis();
            
            if (consecutiveCount >= 3) { // 3 consecutive reads
                uid = currentUid;
                return true;
            }
        }
    } else {
        // No tag present
        if (tagPresent) {
            // Tag was removed
            tagPresent = false;
            lastUid = "";
            consecutiveCount = 0;
        }
    }
    
    return false;
}

// Compute unassigned paths
void SetupMode::computeUnassignedPaths() {
    std::vector<String> unassigned;
    
    for (const String& path : unassignedPaths) {
        String dummy;
        if (!mappingStore->getUidFor(path, dummy)) {
            unassigned.push_back(path);
        }
    }
    
    unassignedPaths = unassigned;
}

// Show prompt message
void SetupMode::showPrompt(const String& message) {
    LOG_SETUP_INFO("%s", message.c_str());
}

// Show status message
void SetupMode::showStatus(const String& message) {
    LOG_SETUP_INFO("%s", message.c_str());
}

// Get state name for debugging
String SetupMode::getCurrentStateName() const {
    switch (currentState) {
        case SetupState::IDLE: return "IDLE";
        case SetupState::SETUP_INIT: return "SETUP_INIT";
        case SetupState::PROMPT_PRESENT_TAG: return "PROMPT_PRESENT_TAG";
        case SetupState::READ_UID: return "READ_UID";
        case SetupState::CONFIRM_OVERWRITE: return "CONFIRM_OVERWRITE";
        case SetupState::WAIT_FOR_CARD_REMOVAL: return "WAIT_FOR_CARD_REMOVAL";
        case SetupState::RFID_SETUP_SUMMARY: return "RFID_SETUP_SUMMARY";
        default: return "UNKNOWN";
    }
}

// Print status for debugging
void SetupMode::printStatus() const {
    LOG_SETUP_DEBUG("SetupMode Status: Active=%s, State=%s, PathIndex=%d/%d",
                    isActive ? "yes" : "no", 
                    getCurrentStateName().c_str(),
                    currentPathIndex,
                    unassignedPaths.size());
}
