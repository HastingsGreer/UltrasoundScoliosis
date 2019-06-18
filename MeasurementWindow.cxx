/*

Library:   UltrasoundIntersonApps

Copyright 2010 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

======================================================================== = */

#include "MeasurementWindow.h"
#include "ITKQtHelpers.hxx"
#include "itkSpectra1DSupportWindowImageFilter.h"
#include "itkMetaDataObject.h"
#include "itkCurvilinearArraySpecialCoordinatesImage.h"

template <class ImageType>
typename ImageType::Pointer CreateImage(int wx, int wy)
{
	ImageType::Pointer image = ImageType::New();

	ImageType::IndexType imageIndex;
	imageIndex.Fill(0);

	ImageType::SizeType imageSize;
	imageSize[0] = wx;
	imageSize[1] = wy;

	ImageType::RegionType imageRegion;
	imageRegion.SetIndex(imageIndex);
	imageRegion.SetSize(imageSize);

	image->SetRegions(imageRegion);
	image->Allocate();

	return image;
}

MeasurementWindow::MeasurementWindow(QGridLayout* my_ui, QLabel* my_graph) {
	m_graph = my_graph;
	m_ui = my_ui;
	m_StatsFilter = StatisticsImageFilterType::New();
	m_BmodeROIFilter = BmodeROIFilterType::New();
	m_RFROIFilter = RFROIFilterType::New();

	m_CastFilter = CastDoubleFilterType::New();
	m_SpectraFilter = SpectraFilterType::New();
	m_WindowFilter = WindowFilterType::New();

	xsize = 128, ysize = 17;

	/*
	m_label = new QLabel("Horse");
	m_label->setParent(m_graph->parentWidget());
	m_label->setGeometry(m_graph->geometry());
	*/

	ImageType::Pointer sideLines = CreateImage<ImageType>(256, 20);
	sideLines->FillBuffer(5);
	m_WindowFilter->SetInput(sideLines);
	m_WindowFilter->SetFFT1DSize(160);
	m_WindowFilter->SetStep(1);
	m_WindowFilter->UpdateLargestPossibleRegion();
	m_WindowFilter->Update();
	std::cout << "the pixel" << std::endl;
	for (auto val : m_WindowFilter->GetOutput()->GetPixel({ 128, 10 })) {
		std::cout << val << std::endl;
	}

	auto window = CreateImage<WindowFilterType::OutputImageType>(32, 2);

	for (int i = 0; i < ysize * 2; i++) {
		window->GetPixel({ 0, 0 }).push_back({ 0, i });
	}
	auto dict = window->GetMetaDataDictionary();
	itk::EncapsulateMetaData<unsigned int>(dict,
		"FFT1DSize", 256);

	m_SpectraFilter->SetSupportWindowImage(window);
}

	

void MeasurementWindow::UpdateMeasurements(IntersonArrayDeviceRF::RFImageType::Pointer rf, ImageType::Pointer bmode) {
    //draw graph
	
	int vres = 70;
	ImageType::Pointer graph = CreateImage<ImageType>(region[1] - region[0], vres);
	graph->FillBuffer(127);

	if (graphPowerSpectrum) {

		m_RFROIFilter->SetInput(rf);
		m_CastFilter->SetInput(m_RFROIFilter->GetOutput());

		m_CastFilter->Update();
		/*
		std::cout << "casted image" << std::endl;
		m_CastFilter->GetOutput()->Print(std::cout);
		std::cout << "that was it" << std::endl;
		*/
		m_SpectraFilter->SetInput(m_CastFilter->GetOutput());

		m_SpectraFilter->Update();
		
		//std::cout << "Spectra:" << std::endl;
		//m_SpectraFilter->GetOutput()->Print(std::cout);
		



	} else {
       //graph literal rf values
		for (int l = region[0]; l < region[1]; l++) {
			double the_value = (double)rf->GetPixel({ l, (region[2] + region[3]) / 2 });
			max = std::max(the_value, max);
			min = std::min(the_value, min);
		}

		for (int l = region[0]; l < region[1]; l++) {
			double the_value = rf->GetPixel({ l, (region[2] + region[3]) / 2 });
			the_value = vres * (the_value - min) / (max - min);
			int val = (int)the_value;
			for (int w = 0; w < vres; w++) {
				graph->SetPixel({ l - region[0], w }, 255 * (w > val));
			}
		}
	}

	//Bmode intensity and stddev
	m_BmodeROIFilter->SetInput(bmode);
	m_StatsFilter->SetInput(m_BmodeROIFilter->GetOutput());
	m_StatsFilter->Update();
	((QLabel *)(this->m_ui->itemAtPosition(2, 1)->widget()))->setText(std::to_string(m_StatsFilter->GetMean()).c_str());

	((QLabel *)(this->m_ui->itemAtPosition(1, 1)->widget()))->setText(std::to_string(m_StatsFilter->GetSigma()).c_str());

	QImage image = ITKQtHelpers::GetQImageColor<ImageType>(
		graph,
		graph->GetLargestPossibleRegion(),
		QImage::Format_RGB16
		);
//	std::cout << "made graph qimage" << std::endl;
	m_graph->setPixmap(QPixmap::fromImage(image));
	m_graph->setScaledContents(true);
	m_graph->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

}

bool is_near_edge(int min, int max, int val) {
	int width = max - min;
	val = val - min;
	float scaled = val / (float)width;
	return scaled > .92 || scaled < .08;
}

void MeasurementWindow::DrawRectangle(itk::VectorImage<double, 2>::Pointer composite, itk::CurvilinearArraySpecialCoordinatesImage<double, 2>::Pointer curvedImage, int index) {

	for (double i = region[0]; i < region[1]; i+= 2) {
		for (double j = region[2]; j < region[3]; j+= .1) {
			if (is_near_edge(region[0], region[1], i) || is_near_edge(region[2], region[3], j)) {
				itk::Point<double, 2> the_point;

				const double idx_pt[2] = { i, j };

				curvedImage->TransformContinuousIndexToPhysicalPoint<double, double>(itk::ContinuousIndex<double, 2>(idx_pt), the_point);

				auto point_index = composite->TransformPhysicalPointToIndex<double>(the_point);
				if (composite->GetLargestPossibleRegion().IsInside(point_index) ){
					auto val = composite->GetPixel(point_index);
					val.SetElement(index, 255);
					composite->SetPixel(point_index, val);
				}
			}
		}
	}
}

void MeasurementWindow::SetRegion(int x, int y) {
	
    region[0] = std::max(0, x - xsize);
    region[1] = std::min(2048, x + xsize);
    region[2] = std::max(0, y -  ysize);
    region[3] = std::min(127, y +  ysize);

	ImageType::SizeType size;

	size[0] = region[1] - region[0];
	size[1] = region[3] - region[2];

	ImageType::IndexType index;
	index[0] = region[0];
	index[1] = region[2];

	m_ITKRegion.SetIndex(index);
	m_ITKRegion.SetSize(size);

	m_BmodeROIFilter->SetRegionOfInterest(m_ITKRegion);
	m_RFROIFilter->SetRegionOfInterest(m_ITKRegion);

	//m_StatsFilter->SetRegionOf
}