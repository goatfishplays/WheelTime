/**
 * @file Menu.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "App/Menu.hpp"
#include "App/Action.hpp"
#include <Platform/Inputs.hpp>
#include <vector>
#include <string>

using namespace Application;

Menu::Menu(Platform::InputBind *_inputBind = nullptr,
           bool _executeOnRelease = false,
           bool _exitOnAction = false,
           std::vector<Action> _actions = {}) : inputBind(_inputBind), executeOnRelease(_executeOnRelease), exitOnAction(_exitOnAction), actions(_actions)
{
}

Menu::~Menu() {};

void Menu::addAction(int ind, Action action) {}

void Menu::remAction(int ind) {}

int Menu::numActions() { return this->actions.size(); }

void Menu::save() {}

void Menu::load() {}