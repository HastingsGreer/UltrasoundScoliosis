/*=========================================================================

Library:   UltrasoundSpectroscopyRecorder

Copyright 2010 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=========================================================================*/


#pragma once

#ifndef SPECTROSCOPYUI_H 
#define SPECTROSCOPYUI_H 

// Print debug statements
// Using print because I am to inept to setup visual studio
//#define DEBUG_PRINT

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <QCheckBox>
#include <qgroupbox.h>
#include <QCloseEvent>

#include "ui_Spectroscopy.h"
#include "IntersonArrayDeviceRF.hxx"

#include "itkBModeImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkButterworthBandpass1DFilterFunction.h"

//Forward declaration of Ui::MainWindow;
namespace Ui{
	class MainWindow;
}

//Declaration of OpticNerveUI
class SpectroscopyUI: public QMainWindow
{
	Q_OBJECT

public:
  SpectroscopyUI(int bufferSize, QWidget *parent = nullptr);
  ~SpectroscopyUI();

protected:
  void  closeEvent(QCloseEvent * event);

protected slots:
  /** Quit the application */
  void ConnectProbe();
  /** Start the application */
  void SetFrequency();
  void SetDepth();
 
  void SetLowerFrequency();
  void SetUpperFrequency();
  void SetOrder();
 
  /** Update the images displayed from the probe */
  void UpdateImage();

  void RecordRF(); 
 
private:
  /** Layout for the Window */
  Ui::MainWindow *ui;
  QTimer *timer;
  std::vector< QCheckBox * > freqCheckBoxes; 
 
  IntersonArrayDeviceRF intersonDevice;
  int lastBModeRendered;
  int lastRFRendered;

  typedef IntersonArrayDeviceRF::RFImageType  RFImageType;
  
  typedef itk::Image<double, 2> ImageType;
  typedef itk::CastImageFilter<RFImageType, ImageType> CastFilter;
  CastFilter::Pointer m_CastFilter;
   
  typedef itk::BModeImageFilter< ImageType >  BModeImageFilter;
  BModeImageFilter::Pointer m_BModeFilter;
  typedef itk::ButterworthBandpass1DFilterFunction ButterworthBandpassFilter;
  ButterworthBandpassFilter::Pointer m_BandpassFilter;
};

#endif
