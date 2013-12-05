#pragma once

#include "pxar_rpc.h"

class pXarAPI
{

 public:
  pXarAPI(std::string name = "*");
  //~pXarAPI();

  void Configure();

 private:
  void PrintInfo();
  void CheckCompatibility();

  CTestboard * tb;
  std::string board_id;

};
