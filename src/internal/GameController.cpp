#include "GameController.h"

#include <iostream>
#include "../include/utils.h"
using namespace std;

GameController::GameController() :
  _autoMode(true), _ended(false), _ending(false),
   _currentPlayerTurn(0), _delayTime(100),
	_model(NULL), _view(NULL), _tileManager(NULL), _appConfig(NULL) {}


GameController::GameController(const GameController& g) {}

GameController& GameController::operator=(const GameController& g) {
  return *this;
}

#pragma region IControllerImplementation
void GameController::setDisplay(CImgDisplay* display) {
  _view->setDisplay(display);
}

string GameController::getConfig(string key) const {
  return _appConfig->getConfig(key);
}

bool GameController::registerPlayer(IPlayer* player) {
  IPlayer* newPlayer = _model->registerPlayer(player);

  if (newPlayer != NULL) {
    _players.push_back(newPlayer);
    return true;
  }

  return false;
}

bool GameController::nextTurn() {
  
  // reduce down:
  int totalPlayer = _players.size();
  if (_currentPlayerTurn >= totalPlayer)
      _currentPlayerTurn %= totalPlayer;

  IPlayer* currentPlayer = _players[_currentPlayerTurn++];

  Command nextMove = currentPlayer->nextMove();

  string nextMoveMessage = currentPlayer->getPlayerInfo()->getPlayerMapID() 
          + string(": ") + nextMove.toString();

  _model->nextTurnCount();

  std::cout << "-- Turn " << _model->getCurrentTurnCount() << " ---" << endl;
  
  if (_model->isValidMove(currentPlayer->getPlayerInfo(), nextMove)) {

    animateMove(nextMove);

    vector<pair<int,int> > changes 
      = _model->applyMove(currentPlayer->getPlayerInfo(), nextMove);
      
    _view->update(changes);
    _view->updateInfo();

    _ending = _model->isEndGame();

    cout << nextMoveMessage << std::endl;

    return true;
  } else {

    cout << nextMoveMessage << " INVALID!!! - SKIP" << std::endl;

    return false;
  }
}



bool GameController::start() {
  // init players
  for (int i = 0; i < _players.size(); i++) {
    _players[i]->onStart();
  }

  _currentPlayerTurn = 0;
  // init views
  _view->initDisplay();
  _view->display();
  
  return true;
}

bool GameController::finish() {
  for (int i = 0; i < _players.size(); i++) {
    _players[i]->onFinish();
  }
  _ended = true;
  return _ended;
}

void GameController::toggleMode() {
  _autoMode = !_autoMode;
}

bool GameController::isInAutoMode() const {
  return _autoMode;
}

bool GameController::isEnding() const {
  return _ending;
}

bool GameController::isEnded() const {
  return _ended;
}

void GameController::updateDisplay() {
  _view->display();
}

GameController::~GameController() {
  delete _appConfig;
  delete _tileManager;
  delete _model;
  delete _view;
}

#pragma endregion

#pragma region InternalPreservedInterfaces

void GameController::setConfig(AppConfig* config) {
  _appConfig = config;
  _delayTime = Utils::parseInt(_appConfig->getConfig("delay"));
}

void GameController::setTileManager(TileManager* tileManager) {
  _tileManager = tileManager;
}

void GameController::createGameModel() {
  if (_model != NULL) {
    delete _model;
    _model = NULL;
  }

  MapInfo mapInfo;

  MapLoader::loadMap(_appConfig->getConfig("map"), mapInfo);

  _model = new BaseGameModel(mapInfo);
}

void GameController::createGameView() {

  if (_view != NULL) {
    delete _view;
    _view = NULL;
  }

  _view = new BaseGameView(_tileManager, _model);
}


void GameController::animateMove(const Command& move) {
  switch (move.getActionType()) {
    case Command::SURRENDER:
    case Command::SKIP:
    case Command::MOVE:
      break;

    case Command::FIRE: {
      pair<int,int> pos = move.getTargetPosition();
      pair<int,int> from = move.getReceivingObject()->getPosition();

      int deltaX = pos.first - from.first;
      int deltaY = pos.second - from.second;

      if (deltaX == 0) {
        int dy = deltaY > 0 ? 1 : -1;

        for (int i = from.second + dy; i != pos.second; i+=dy){
          _view->addFire(pos.first, i, "EFFECT.BULLET_M");
          _view->display();
          _view->getDisplay()->wait(_delayTime);
          _view->removeFire(pos.first, i);
        }

      } else if (deltaY == 0) {
        int dx = deltaX > 0 ? 1 : -1;

        for (int i = from.first + dx; i != pos.first; i+=dx){
          _view->addFire(i, pos.second, "EFFECT.BULLET_M");
          _view->display();
          _view->getDisplay()->wait(_delayTime);
          _view->removeFire(i, pos.second);
        }

      } else {

      }

      _view->addFire(pos.first, pos.second, "EFFECT.FIRE_M");
      _view->display();
      _view->getDisplay()->wait(_delayTime);
      _view->removeFire(pos.first, pos.second);
      _view->display();
    }
  }
}
