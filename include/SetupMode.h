#ifndef SETUP_MODE_H
#define SETUP_MODE_H

#include <Arduino.h>
#include <vector>
#include "MappingStore.h"
#include "SdScanner.h"
#include "RFID_Manager.h"
#include "Button_Manager.h"

// Setup mode states
enum class SetupState {
    IDLE,
    SETUP_INIT,
    RFID_SETUP_LOOP,
    PROMPT_PRESENT_TAG,
    READ_UID,
    CONFIRM_OVERWRITE,
    WAIT_FOR_CARD_REMOVAL,
    RFID_SETUP_SUMMARY
};

// Button actions for setup mode
enum class SetupButtonAction {
    NONE,
    PLAY_OK,      // confirm
    BACK_PREV,    // cancel/deny
    NEXT,         // skip
    LONG_PRESS    // enter/exit setup mode
};

class SetupMode {
private:
    // State management
    SetupState currentState;
    bool isActive;
    
    // Components
    MappingStore* mappingStore;
    SdScanner* sdScanner;
    RFID_Manager* rfidManager;
    Button_Manager* buttonManager;
    
    // Setup data
    std::vector<String> unassignedPaths;
    size_t currentPathIndex;
    String currentFolder;
    String currentUid;
    String contentRoot;  // Configurable content root
    
    // Assignment tracking
    int assignedCount;
    int skippedCount;
    int totalProcessed;
    
    // Timing
    unsigned long setupStartTime;
    unsigned long lastRfidRead;
    unsigned long lastButtonCheck;
    const unsigned long RFID_TIMEOUT = 30000;        // 30 seconds
    const unsigned long BUTTON_DEBOUNCE = 200;       // 200ms
    const unsigned long LONG_PRESS_THRESHOLD = 800;  // 800ms
    
    // Button state
    bool encoderPressed;
    unsigned long encoderPressStart;
    SetupButtonAction lastButtonAction;
    ButtonType lastButtonState;
    
    // FSM methods
    void stepInit();
    void stepPrompt();
    void stepReadUid();
    void stepConfirmOverwrite();
    void stepWaitForCardRemoval();
    void stepSummary();
    
    // Helper methods
    void handleUid(const String& uid);
    void handleButtonAction(SetupButtonAction action);
    SetupButtonAction getButtonAction();
    bool waitForRfid(String& uid, unsigned long timeout);
    void showPrompt(const String& message);
    void showStatus(const String& message);
    void computeUnassignedPaths();
    
public:
    SetupMode();
    
    // Initialization
    bool begin(MappingStore* store, SdScanner* scanner, RFID_Manager* rfid, Button_Manager* buttons, const String& root = "/test_music");
    bool isInitialized() const;
    
    // Main control
    void enter();
    void exit();
    void loop();  // call while in setup mode
    bool isSetupActive() const { return isActive; }
    
    // State queries
    SetupState getCurrentState() const { return currentState; }
    String getCurrentStateName() const;
    
    // Configuration
    void setContentRoot(const String& root) { contentRoot = root; }
    String getContentRoot() const { return contentRoot; }
    bool loadContentRootFromSettings(const char* settingsKey = "content_root");
    
    // Debug
    void printStatus() const;
};

#endif // SETUP_MODE_H
