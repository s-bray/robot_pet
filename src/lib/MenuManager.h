#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <vector>

enum MenuItemType
{
  ACTION,  // Item yang bisa dipilih untuk eksekusi aksi
  TOGGLE,  // Item on/off
  SUBMENU, // Item yang membuka submenu
  INFO     // Item hanya untuk menampilkan info
};

struct MenuItem
{
  String label;
  MenuItemType type;
  bool *toggleState;                // Pointer ke boolean untuk TOGGLE
  std::function<void()> action;     // Callback untuk ACTION
  std::vector<MenuItem> *submenu;   // Pointer ke submenu items
  std::function<String()> getValue; // Callback untuk mendapatkan nilai dinamis (untuk INFO)

  MenuItem(String lbl, MenuItemType t = ACTION)
      : label(lbl), type(t), toggleState(nullptr), action(nullptr), submenu(nullptr), getValue(nullptr) {}
};

class MenuManager
{
private:
  Adafruit_SSD1306 &display;
  std::vector<MenuItem> mainMenu;
  std::vector<MenuItem> *currentMenu;
  std::vector<std::vector<MenuItem> *> menuStack; // Stack untuk navigasi menu
  std::vector<String> menuTitleStack;             // Stack untuk title menu

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

  // Fungsi untuk menambahkan item ke main menu
  void addItem(const MenuItem &item);
  void addItem(String label, MenuItemType type, std::function<void()> callback);
  void addToggleItem(String label, bool *toggleState, std::function<void(bool)> callback = nullptr);
  void addSubmenu(String label, std::vector<MenuItem> *submenu);
  void addInfoItem(String label, std::function<String()> getValue);

  // Fungsi untuk navigasi menu
  void show();
  void hide();
  void update();
  void navigateDown();
  void navigateUp();
  void selectItem();
  void back(); // Fungsi untuk kembali ke menu sebelumnya

  bool isMenuActive() { return isActive; }
  void setMenuTitle(String title) { currentMenuTitle = title; }

  // Helper untuk membuat dan mengisi submenu
  std::vector<MenuItem> *createSubmenu();
  void addItemToSubmenu(std::vector<MenuItem> *submenu, const MenuItem &item);

  // Helper khusus untuk menambahkan berbagai tipe item ke submenu
  void addActionToSubmenu(std::vector<MenuItem> *submenu, String label, std::function<void()> callback);
  void addToggleToSubmenu(std::vector<MenuItem> *submenu, String label, bool *toggleState, std::function<void(bool)> callback = nullptr);
  void addSubmenuToSubmenu(std::vector<MenuItem> *parentSubmenu, String label, std::vector<MenuItem> *childSubmenu);
  void addInfoToSubmenu(std::vector<MenuItem> *submenu, String label, std::function<String()> getValue);
};

#endif