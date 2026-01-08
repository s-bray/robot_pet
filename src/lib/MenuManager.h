#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <vector>

enum MenuItemType
{
  ACTION,  // Item that can be selected to execute action
  TOGGLE,  // Item on/off
  SUBMENU, // Item that opens a submenu
  INFO     // Item detailing info only
};

struct MenuItem
{
  String label;
  MenuItemType type;
  bool *toggleState;                // Pointer to boolean for TOGGLE
  std::function<void()> action;     // Callback for ACTION
  std::vector<MenuItem> *submenu;   // Pointer to submenu items
  std::function<String()> getValue; // Callback to get dynamic value (for INFO)

  MenuItem(String lbl, MenuItemType t = ACTION)
      : label(lbl), type(t), toggleState(nullptr), action(nullptr), submenu(nullptr), getValue(nullptr) {}
};

class MenuManager
{
private:
  Adafruit_SSD1306 &display;
  std::vector<MenuItem> mainMenu;
  std::vector<MenuItem> *currentMenu;
  std::vector<std::vector<MenuItem> *> menuStack; // Stack for menu navigation
  std::vector<String> menuTitleStack;             // Stack for menu titles

  int selectedIndex;
  int scrollOffset;
  int maxVisibleItems;
  bool isActive;

  const int MENU_TITLE_HEIGHT = 12;
  const int ITEM_HEIGHT = 13;
  const int LEFT_MARGIN = 2;
  const int RIGHT_MARGIN = 2;

  String currentMenuTitle;

  void drawMenu();
  void drawMenuItem(int y, const MenuItem &item, bool isSelected);
  void executeSelectedItem();
  void enterSubmenu();
  void exitMenu();
  int getMaxVisibleItems();

public:
  MenuManager(Adafruit_SSD1306 &disp);

  void begin();

  // Function to add item to main menu
  void addItem(const MenuItem &item);
  void addItem(String label, MenuItemType type, std::function<void()> callback);
  void addToggleItem(String label, bool *toggleState, std::function<void(bool)> callback = nullptr);
  void addSubmenu(String label, std::vector<MenuItem> *submenu);
  void addInfoItem(String label, std::function<String()> getValue);

  // Function for menu navigation
  void show();
  void hide();
  void update();
  void navigateDown();
  void navigateUp();
  void selectItem();
  void back(); // Function to return to previous menu

  bool isMenuActive() { return isActive; }
  void setMenuTitle(String title) { currentMenuTitle = title; }

  // Helper to create and populate submenu
  std::vector<MenuItem> *createSubmenu();
  void addItemToSubmenu(std::vector<MenuItem> *submenu, const MenuItem &item);

  // Special helper to add various item types to submenu
  void addActionToSubmenu(std::vector<MenuItem> *submenu, String label, std::function<void()> callback);
  void addToggleToSubmenu(std::vector<MenuItem> *submenu, String label, bool *toggleState, std::function<void(bool)> callback = nullptr);
  void addSubmenuToSubmenu(std::vector<MenuItem> *parentSubmenu, String label, std::vector<MenuItem> *childSubmenu);
  void addInfoToSubmenu(std::vector<MenuItem> *submenu, String label, std::function<String()> getValue);
};

#endif