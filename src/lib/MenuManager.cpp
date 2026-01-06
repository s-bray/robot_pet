#include "MenuManager.h"

MenuManager::MenuManager(Adafruit_SSD1306 &disp)
    : display(disp), selectedIndex(0), scrollOffset(0), isActive(false),
      currentMenu(&mainMenu), currentMenuTitle("Menu")
{
  maxVisibleItems = getMaxVisibleItems();
}

void MenuManager::begin()
{
  Serial.println("[MenuManager] Initialized");
}

int MenuManager::getMaxVisibleItems()
{
  int availableHeight = display.height() - MENU_TITLE_HEIGHT - 2;
  return availableHeight / ITEM_HEIGHT;
}

void MenuManager::addItem(const MenuItem &item)
{
  mainMenu.push_back(item);
}

void MenuManager::addItem(String label, MenuItemType type, std::function<void()> callback)
{
  MenuItem item(label, type);
  item.action = callback;
  mainMenu.push_back(item);
}

void MenuManager::addToggleItem(String label, bool *toggleState, std::function<void(bool)> callback)
{
  MenuItem item(label, TOGGLE);
  item.toggleState = toggleState;
  if (callback)
  {
    item.action = [toggleState, callback]()
    {
      *toggleState = !(*toggleState);
      callback(*toggleState);
    };
  }
  else
  {
    item.action = [toggleState]()
    {
      *toggleState = !(*toggleState);
    };
  }
  mainMenu.push_back(item);
}

void MenuManager::addSubmenu(String label, std::vector<MenuItem> *submenu)
{
  MenuItem item(label, SUBMENU);
  item.submenu = submenu;
  mainMenu.push_back(item);
}

void MenuManager::addInfoItem(String label, std::function<String()> getValue)
{
  MenuItem item(label, INFO);
  item.getValue = getValue;
  mainMenu.push_back(item);
}

std::vector<MenuItem> *MenuManager::createSubmenu()
{
  return new std::vector<MenuItem>();
}

void MenuManager::addItemToSubmenu(std::vector<MenuItem> *submenu, const MenuItem &item)
{
  if (submenu)
  {
    submenu->push_back(item);
  }
}

void MenuManager::addActionToSubmenu(std::vector<MenuItem> *submenu, String label, std::function<void()> callback)
{
  if (submenu)
  {
    MenuItem item(label, ACTION);
    item.action = callback;
    submenu->push_back(item);
  }
}

void MenuManager::addToggleToSubmenu(std::vector<MenuItem> *submenu, String label, bool *toggleState, std::function<void(bool)> callback)
{
  if (submenu)
  {
    MenuItem item(label, TOGGLE);
    item.toggleState = toggleState;
    if (callback)
    {
      item.action = [toggleState, callback]()
      {
        *toggleState = !(*toggleState);
        callback(*toggleState);
      };
    }
    else
    {
      item.action = [toggleState]()
      {
        *toggleState = !(*toggleState);
      };
    }
    submenu->push_back(item);
  }
}

void MenuManager::addSubmenuToSubmenu(std::vector<MenuItem> *parentSubmenu, String label, std::vector<MenuItem> *childSubmenu)
{
  if (parentSubmenu && childSubmenu)
  {
    MenuItem item(label, SUBMENU);
    item.submenu = childSubmenu;
    parentSubmenu->push_back(item);
  }
}

void MenuManager::addInfoToSubmenu(std::vector<MenuItem> *submenu, String label, std::function<String()> getValue)
{
  if (submenu)
  {
    MenuItem item(label, INFO);
    item.getValue = getValue;
    submenu->push_back(item);
  }
}

void MenuManager::show()
{
  if (isActive)
    return;

  isActive = true;
  selectedIndex = 0;
  scrollOffset = 0;
  currentMenu = &mainMenu;
  menuStack.clear();
  menuTitleStack.clear();
  currentMenuTitle = "Menu";

  Serial.println("[MenuManager] Menu opened");
  drawMenu();
}

void MenuManager::hide()
{
  if (!isActive)
    return;

  isActive = false;
  currentMenu = &mainMenu;
  menuStack.clear();
  menuTitleStack.clear();

  display.clearDisplay();
  display.display();

  Serial.println("[MenuManager] Menu closed");
}

void MenuManager::update()
{
  if (!isActive)
    return;

  drawMenu();
}

void MenuManager::navigateDown()
{
  if (!isActive || currentMenu->empty())
    return;

  selectedIndex++;
  if (selectedIndex >= (int)currentMenu->size())
  {
    selectedIndex = 0;
    scrollOffset = 0;
  }
  else if (selectedIndex >= scrollOffset + maxVisibleItems)
  {
    scrollOffset++;
  }

  Serial.print("[MenuManager] Navigate down -> ");
  Serial.println(selectedIndex);
  drawMenu();
}

void MenuManager::navigateUp()
{
  if (!isActive || currentMenu->empty())
    return;

  selectedIndex--;
  if (selectedIndex < 0)
  {
    selectedIndex = currentMenu->size() - 1;
    scrollOffset = max(0, (int)currentMenu->size() - maxVisibleItems);
  }
  else if (selectedIndex < scrollOffset)
  {
    scrollOffset--;
  }

  Serial.print("[MenuManager] Navigate up -> ");
  Serial.println(selectedIndex);
  drawMenu();
}

void MenuManager::selectItem()
{
  if (!isActive || currentMenu->empty())
    return;

  executeSelectedItem();
}

void MenuManager::back()
{
  if (!isActive)
    return;

  exitMenu();
}

void MenuManager::executeSelectedItem()
{
  if (selectedIndex < 0 || selectedIndex >= (int)currentMenu->size())
    return;

  MenuItem &item = (*currentMenu)[selectedIndex];

  Serial.print("[MenuManager] Selected: ");
  Serial.println(item.label);

  switch (item.type)
  {
  case ACTION:
    if (item.action)
    {
      item.action();
    }
    break;

  case TOGGLE:
    if (item.action)
    {
      item.action();
      drawMenu(); // Refresh untuk menampilkan state baru
    }
    break;

  case SUBMENU:
    enterSubmenu();
    break;

  case INFO:
    // Info items tidak bisa diselect
    break;
  }
}

void MenuManager::enterSubmenu()
{
  if (selectedIndex < 0 || selectedIndex >= (int)currentMenu->size())
    return;

  MenuItem &item = (*currentMenu)[selectedIndex];

  if (item.type == SUBMENU && item.submenu && !item.submenu->empty())
  {
    // Simpan menu dan title saat ini
    menuStack.push_back(currentMenu);
    menuTitleStack.push_back(currentMenuTitle);

    // Pindah ke submenu
    currentMenu = item.submenu;
    currentMenuTitle = item.label;
    selectedIndex = 0;
    scrollOffset = 0;

    Serial.print("[MenuManager] Enter submenu: ");
    Serial.println(item.label);

    drawMenu();
  }
}

void MenuManager::exitMenu()
{
  if (menuStack.empty())
  {
    hide();
  }
  else
  {
    // Kembali ke menu sebelumnya
    currentMenu = menuStack.back();
    menuStack.pop_back();

    // Restore title
    if (!menuTitleStack.empty())
    {
      currentMenuTitle = menuTitleStack.back();
      menuTitleStack.pop_back();
    }
    else
    {
      currentMenuTitle = "Menu";
    }

    selectedIndex = 0;
    scrollOffset = 0;

    Serial.println("[MenuManager] Back to previous menu");
    drawMenu();
  }
}

void MenuManager::drawMenu()
{
  display.clearDisplay();

  // Draw title bar
  display.fillRect(0, 0, display.width(), MENU_TITLE_HEIGHT, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(2, 2);

  // Truncate title if too long
  String displayTitle = currentMenuTitle;
  if (displayTitle.length() > 20)
  {
    displayTitle = displayTitle.substring(0, 17) + "...";
  }
  display.print(displayTitle);

  // Draw separator line
  display.drawLine(0, MENU_TITLE_HEIGHT, display.width(), MENU_TITLE_HEIGHT, SSD1306_WHITE);

  // Draw menu items
  display.setTextColor(SSD1306_WHITE);
  int y = MENU_TITLE_HEIGHT + 10;

  int startIdx = scrollOffset;
  int endIdx = min(scrollOffset + maxVisibleItems, (int)currentMenu->size());

  for (int i = startIdx; i < endIdx; i++)
  {
    bool isSelected = (i == selectedIndex);
    drawMenuItem(y, (*currentMenu)[i], isSelected);
    y += ITEM_HEIGHT;
  }

  // Draw scroll indicators
  if (scrollOffset > 0)
  {
    // Up arrow
    display.fillTriangle(
        display.width() - 8, MENU_TITLE_HEIGHT + 4,
        display.width() - 4, MENU_TITLE_HEIGHT + 4,
        display.width() - 6, MENU_TITLE_HEIGHT + 2,
        SSD1306_WHITE);
  }

  if (scrollOffset + maxVisibleItems < (int)currentMenu->size())
  {
    // Down arrow
    display.fillTriangle(
        display.width() - 8, display.height() - 4,
        display.width() - 4, display.height() - 4,
        display.width() - 6, display.height() - 2,
        SSD1306_WHITE);
  }

  display.display();
}

void MenuManager::drawMenuItem(int y, const MenuItem &item, bool isSelected)
{
  // Draw selection indicator
  if (isSelected)
  {
    display.setCursor(LEFT_MARGIN, y);
    display.print(">");
  }

  // Draw item label
  display.setCursor(LEFT_MARGIN + 8, y);

  // Truncate label if too long (max ~15 chars for 128px width)
  String displayLabel = item.label;
  if (displayLabel.length() > 15)
  {
    displayLabel = displayLabel.substring(0, 12) + "...";
  }
  display.print(displayLabel);

  // Draw item value/state
  int valueX = display.width() - RIGHT_MARGIN;
  String valueStr = "";

  switch (item.type)
  {
  case TOGGLE:
    if (item.toggleState)
    {
      valueStr = *item.toggleState ? "ON" : "OFF";
    }
    break;

  case SUBMENU:
    valueStr = ">";
    break;

  case INFO:
    if (item.getValue)
    {
      valueStr = item.getValue();
    }
    break;

  default:
    break;
  }

  if (valueStr.length() > 0)
  {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(valueStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(valueX - w, y);
    display.print(valueStr);
  }
}