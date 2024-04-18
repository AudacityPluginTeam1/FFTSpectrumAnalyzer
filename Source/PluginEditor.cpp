/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <string>
#include <algorithm>

float FFTSpectrumAnalyzerAudioProcessorEditor::cursorX;
int FFTSpectrumAnalyzerAudioProcessorEditor::cursorPeak = 0;
bool FFTSpectrumAnalyzerAudioProcessorEditor::isRunning = false;
bool FFTSpectrumAnalyzerAudioProcessorEditor::newSelection = false;
bool FFTSpectrumAnalyzerAudioProcessorEditor::initialWindow = true;
bool FFTSpectrumAnalyzerAudioProcessorEditor::isGraph = false;
bool FFTSpectrumAnalyzerAudioProcessorEditor::isVisiblePlot1 = true;
bool FFTSpectrumAnalyzerAudioProcessorEditor::isVisiblePlot2 = true;
float FFTSpectrumAnalyzerAudioProcessorEditor::xMinPrev = 0;
float FFTSpectrumAnalyzerAudioProcessorEditor::xMin = 0;
float FFTSpectrumAnalyzerAudioProcessorEditor::xMaxPrev = 100;
float FFTSpectrumAnalyzerAudioProcessorEditor::xMax = 8000;
float FFTSpectrumAnalyzerAudioProcessorEditor::yMinPrev = -1;
float FFTSpectrumAnalyzerAudioProcessorEditor::yMin = -90;
float FFTSpectrumAnalyzerAudioProcessorEditor::yMaxPrev = 1;
float FFTSpectrumAnalyzerAudioProcessorEditor::yMax = 0;
int FFTSpectrumAnalyzerAudioProcessorEditor::plotIndexSelection = 0;

int FFTSpectrumAnalyzerAudioProcessorEditor::windowWidth = 950;
int FFTSpectrumAnalyzerAudioProcessorEditor::windowHeight = 550 + 2;
int FFTSpectrumAnalyzerAudioProcessorEditor::windowMaxWidth = 2160;
int FFTSpectrumAnalyzerAudioProcessorEditor::windowMaxHeight = 1080;

// height and width for primary category labels (Import Audio, Zoom, Export, etc.) 
const int width_primaryCategoryLabel = 275;
const int height_primaryCategoryLabel = 25;
// height and width for secondary labels ("Selected Traces", Upper/Lower, etc.)
const int width_secondaryLabel = 150;
const int height_secondaryLabel = 25;
// space between primary labels and secondary labels
const int yOffsetPrimary_secondaryLabel = 8;
// space between secondary components (e.g. white box for plot selection) and physical boundaries
const int x_componentOffset = 6;
const int y_componentOffset = 6;
// dimensions of white box for plot selection
const int yOffset_selectionBox = 2;
const int width_selectionBox = 263;
const int height_selectionBox = 90;

//ROW INDEX STUFF!!!
int FFTSpectrumAnalyzerAudioProcessorEditor::rowSize = 2;
int FFTSpectrumAnalyzerAudioProcessorEditor::rowIndex = 0;
int FFTSpectrumAnalyzerAudioProcessorEditor::count = 0;
int FFTSpectrumAnalyzerAudioProcessorEditor::countPrev = 0;


//Processor statics
int FFTSpectrumAnalyzerAudioProcessorEditor::fftSize = 1024;
int FFTSpectrumAnalyzerAudioProcessorEditor::numBins = 513;
int FFTSpectrumAnalyzerAudioProcessorEditor::maxFreq = 8000;
int FFTSpectrumAnalyzerAudioProcessorEditor::stepSize = 512;
int FFTSpectrumAnalyzerAudioProcessorEditor::numFreqBins = 0;
int FFTSpectrumAnalyzerAudioProcessorEditor::fftCounter = 0;

bool FFTSpectrumAnalyzerAudioProcessorEditor::setToLog = false;

//Processor vectors
std::vector<float> FFTSpectrumAnalyzerAudioProcessorEditor::indexToFreqMap = { 0 };
std::vector< std::vector<float>> FFTSpectrumAnalyzerAudioProcessorEditor::binMag;
std::vector< std::vector<float>> FFTSpectrumAnalyzerAudioProcessorEditor::sampleSelections;
std::vector<float> FFTSpectrumAnalyzerAudioProcessorEditor::bufferRight = { 0 };
std::vector<float> FFTSpectrumAnalyzerAudioProcessorEditor::bufferLeft = { 0 };
std::vector<float> FFTSpectrumAnalyzerAudioProcessorEditor::windowBufferRight = { 0 };
std::vector<float> FFTSpectrumAnalyzerAudioProcessorEditor::windowBufferLeft = { 0 };
//juce::dsp::FFT FFTSpectrumAnalyzerAudioProcessorEditor::editFFT(0);

juce::dsp::WindowingFunction<float> FFTSpectrumAnalyzerAudioProcessorEditor::editWindow(0, juce::dsp::WindowingFunction<float>::blackman);

//==============================================================================
FFTSpectrumAnalyzerAudioProcessorEditor::FFTSpectrumAnalyzerAudioProcessorEditor(FFTSpectrumAnalyzerAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p)
{
	setOpaque(true);
	startTimer(500);

	setSize(windowWidth, windowHeight);
	setResizable(true, true);
	setResizeLimits(windowWidth, windowHeight, windowMaxWidth, windowMaxHeight);

	sampleSelections.resize(2);
	setPlotIndex(plotIndexSelection);
	setFreqData(1024);
	//audioProcessor.zeroAllSelections(numBins, rowSize);
	//audioProcessor.prepBuffers(fftS);
	//binMag = audioProcessor.getBinSet();
	zeroBuffers();

	// new gui elements start
	addAndMakeVisible(gui_importAudio);
	gui_importAudio.setFont(juce::Font("Arial", 18.0f, juce::Font::bold));
	gui_importAudio.setText("Import Audio", juce::dontSendNotification);
	gui_importAudio.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);

	addAndMakeVisible(gui_selectTrace);
	gui_selectTrace.setText("Selected Traces", juce::dontSendNotification);
	gui_selectTrace.setFont(juce::Font(17.0f));

	addAndMakeVisible(gui_zoom);
	gui_zoom.setFont(juce::Font("Arial", 18.0f, juce::Font::bold));
	gui_zoom.setText("Zoom", juce::dontSendNotification);
	gui_zoom.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);

	addAndMakeVisible(gui_upper);
	gui_upper.setText("Upper", juce::dontSendNotification);
	gui_upper.setFont(juce::Font(17.0f));

	addAndMakeVisible(gui_lower);
	gui_lower.setText("Lower", juce::dontSendNotification);
	gui_lower.setFont(juce::Font(17.0f));

	addAndMakeVisible(gui_xUpper);
	gui_xUpper.setText("X", juce::dontSendNotification);
	gui_xUpper.setFont(juce::Font(17.0f));

	addAndMakeVisible(gui_yUpper);
	gui_yUpper.setText("Y", juce::dontSendNotification);
	gui_yUpper.setFont(juce::Font(17.0f));

	addAndMakeVisible(gui_xLower);
	gui_xLower.setText("X", juce::dontSendNotification);
	gui_xLower.setFont(juce::Font(17.0f));

	addAndMakeVisible(gui_yLower);
	gui_yLower.setText("Y", juce::dontSendNotification);
	gui_yLower.setFont(juce::Font(17.0f));

	addAndMakeVisible(gui_export);
	gui_export.setText("Export", juce::dontSendNotification);
	gui_export.setFont(juce::Font("Arial", 18.0f, juce::Font::bold));
	gui_export.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);

	addAndMakeVisible(gui_exportButton);
	gui_exportButton.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
	gui_exportButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
	gui_exportButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);

	addAndMakeVisible(cursorLabel);
	cursorLabel.setText("Cursor", juce::dontSendNotification);
	cursorLabel.setFont(juce::Font("Arial", 14.0f, juce::Font::bold));
	cursorLabel.setColour(juce::Label::textColourId, juce::Colours::white);

	addAndMakeVisible(cursorPlot);
	cursorPlot.setText("(0.0, 0.00)", juce::dontSendNotification);
	cursorPlot.setEditable(false);

	addAndMakeVisible(peakLabel);
	peakLabel.setText("Peak", juce::dontSendNotification);
	peakLabel.setFont(juce::Font("Arial", 14.0f, juce::Font::bold));
	peakLabel.setColour(juce::Label::textColourId, juce::Colours::white);

	addAndMakeVisible(peakPlot);

	addAndMakeVisible(windowLabel);
	windowLabel.setText("Function", juce::dontSendNotification);
	windowLabel.setFont(juce::Font("Arial", 14.0f, juce::Font::bold));
	windowLabel.setColour(juce::Label::textColourId, juce::Colours::white);

	addAndMakeVisible(axisLabel);
	axisLabel.setText("Axis", juce::dontSendNotification);
	axisLabel.setFont(juce::Font("Arial", 14.0f, juce::Font::bold));
	axisLabel.setColour(juce::Label::textColourId, juce::Colours::white);

	addAndMakeVisible(sizeLabel);
	sizeLabel.setText("Size", juce::dontSendNotification);
	sizeLabel.setFont(juce::Font("Arial", 14.0f, juce::Font::bold));
	sizeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

	addAndMakeVisible(windowFunction);
	windowFunction.addItem("Blackman window", 1);
	windowFunction.addItem("Blackman-Harris window", 2);
	windowFunction.addItem("Flatop window", 3);
	windowFunction.addItem("Hamming window", 4);
	windowFunction.addItem("Hann window", 5);
	windowFunction.addItem("Kaiser", 6);
	windowFunction.addItem("Rectangular window", 7);
	windowFunction.addItem("Triangular window", 8);
	windowFunction.setSelectedId(5);
	windowFunction.setColour(juce::ComboBox::backgroundColourId, juce::Colours::white);
	windowFunction.setColour(juce::ComboBox::textColourId, juce::Colours::black);
	windowFunction.setColour(juce::ComboBox::arrowColourId, juce::Colours::darkgrey);
	windowFunction.onChange = [this] { setWindowFunction(); };

	addAndMakeVisible(axis);
	axis.addItem("Linear Frequency", 1);
	axis.addItem("Log Frequency", 2);
	axis.setSelectedId(2);
	axis.setColour(juce::ComboBox::backgroundColourId, juce::Colours::white);
	axis.setColour(juce::ComboBox::textColourId, juce::Colours::black);
	axis.setColour(juce::ComboBox::arrowColourId, juce::Colours::darkgrey);
	axis.onChange = [this] { setAxisType(); };

	addAndMakeVisible(size);
	size.addItem("128", 1);
	size.addItem("256", 2);
	size.addItem("512", 3);
	size.addItem("1024", 4);
	//size.addItem("2048", 5);
	//size.addItem("4096", 6);
	//size.addItem("8192", 7);
	//size.addItem("16384", 8);
	//size.addItem("32768", 9);
	//size.addItem("65536", 10);
	//size.addItem("131072", 11);
	size.setSelectedId(4);
	size.setColour(juce::ComboBox::backgroundColourId, juce::Colours::white);
	size.setColour(juce::ComboBox::textColourId, juce::Colours::black);
	size.setColour(juce::ComboBox::arrowColourId, juce::Colours::darkgrey);
	size.onChange = [this] { setBlockSize(); };


	// new gui elements end

	// buttons to select which of two graphs to plot
	addAndMakeVisible(buttonPlot1);
	buttonPlot1.setClickingTogglesState(true);
	buttonPlot1.onClick = [&]()
		{
			plotIndexSelection = 0;
			setPlotIndex(0);
		};
	addAndMakeVisible(buttonPlot2);
	buttonPlot2.setClickingTogglesState(true);
	buttonPlot2.onClick = [&]()
		{
			plotIndexSelection = 1;
			setPlotIndex(1);
		};

	// toggle button for plot 1
	addAndMakeVisible(toggleButtonPlot1);
	toggleButtonPlot1.setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
	toggleButtonPlot1.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::lightgrey);
	if (isVisiblePlot1 == true)
	{
		toggleButtonPlot1.setToggleState(true, true);
	}
	toggleButtonPlot1.onClick = [this] { updateToggleState(1); };
	toggleButtonPlot1.setClickingTogglesState(true);

	// toggle button for plot 2
	addAndMakeVisible(toggleButtonPlot2);
	toggleButtonPlot2.setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
	toggleButtonPlot2.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::lightgrey);
	if (isVisiblePlot2 == true)
	{
		toggleButtonPlot2.setToggleState(true, true);
	}
	toggleButtonPlot2.onClick = [this] { updateToggleState(2); };
	toggleButtonPlot2.setClickingTogglesState(true);

	addAndMakeVisible(inputXmin);
	addAndMakeVisible(inputXmax);
	addAndMakeVisible(inputYmin);
	addAndMakeVisible(inputYmax);

	addAndMakeVisible(labelPlot1);
	addAndMakeVisible(labelPlot2);

	inputXmin.setEditable(true);
	inputXmax.setEditable(true);
	inputYmin.setEditable(true);
	inputYmax.setEditable(true);
	labelPlot1.setEditable(false);
	labelPlot2.setEditable(false);

	inputXmin.setText(std::to_string(xMin), juce::dontSendNotification);
	inputXmax.setText(std::to_string(xMax), juce::dontSendNotification);
	inputYmin.setText(std::to_string(yMin), juce::dontSendNotification);
	inputYmax.setText(std::to_string(yMax), juce::dontSendNotification);

	labelPlot1.setText("Plot 1", juce::dontSendNotification);
	labelPlot1.setColour(juce::Label::textColourId, juce::Colours::darkgrey);
	labelPlot2.setText("Plot 2", juce::dontSendNotification);
	labelPlot2.setColour(juce::Label::textColourId, juce::Colours::darkgrey);

	inputXmin.setColour(juce::Label::backgroundColourId, juce::Colours::white);
	inputXmin.setColour(juce::Label::textColourId, juce::Colours::black);
	inputXmin.setColour(juce::Label::textWhenEditingColourId, juce::Colours::black);

	inputXmax.setColour(juce::Label::backgroundColourId, juce::Colours::white);
	inputXmax.setColour(juce::Label::textColourId, juce::Colours::black);
	inputXmax.setColour(juce::Label::textWhenEditingColourId, juce::Colours::black);

	inputYmin.setColour(juce::Label::backgroundColourId, juce::Colours::white);
	inputYmin.setColour(juce::Label::textColourId, juce::Colours::black);
	inputYmin.setColour(juce::Label::textWhenEditingColourId, juce::Colours::black);

	inputYmax.setColour(juce::Label::backgroundColourId, juce::Colours::white);
	inputYmax.setColour(juce::Label::textColourId, juce::Colours::black);
	inputYmax.setColour(juce::Label::textWhenEditingColourId, juce::Colours::black);

	inputXmin.onTextChange = [this] { getBounds(); };
	inputXmax.onTextChange = [this] { getBounds(); };
	inputYmin.onTextChange = [this] { getBounds(); };
	inputYmax.onTextChange = [this] { getBounds(); };
}

FFTSpectrumAnalyzerAudioProcessorEditor::~FFTSpectrumAnalyzerAudioProcessorEditor()
{
}

//==============================================================================
void FFTSpectrumAnalyzerAudioProcessorEditor::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);
	g.setOpacity(1.0f);
	g.setColour(juce::Colours::white);

	//PROCESSOR CLASS CODE!!!!!!!!!
	//rowSize = 2;



	//audioProcessor.setFFTSize(fftSize);

	//handleNewSelection(numBins, rowSize, rowIndex);

	//juce::dsp::WindowingFunction<float>::WindowingMethod windowType = juce::dsp::WindowingFunction<float>::WindowingMethod::hann;
	//audioProcessor.setWindow(windowType);


	//audioProcessor.zeroAllSelections(numBins, rowSize);
	//audioProcessor.prepBuffers(fftSize);
	//binMag = audioProcessor.getBinMag();
	//audioProcessor.zeroSelection(rowIndex, numBins);
	if (newSelection == true) {
		/*fftCounter = audioProcessor.getFFTCounter();
		binMag[rowIndex] = audioProcessor.getBinMag();
		audioProcessor.prepBuffers(fftS);
		audioProcessor.zeroSelection(rowIndex);
		audioProcessor.clearRingBuffer();
		sampleSelections[rowIndex] = audioProcessor.getAccumulationBuffer();
		audioProcessor.clearAccumulationBuffer();
		if (fftCounter != 0)
		{
			for (int i = 0; i < numBins; i++) {
				binMag[rowIndex][i] /= fftCounter;
			}
		}*/
		//int sampleRate = audioProcessor.getBlockSampleRate();
		audioProcessor.setStepSize(stepSize);                             //this needs to be changed when the size is changed
		sampleSelections[rowIndex] = audioProcessor.getAccumulationBuffer();
		audioProcessor.clearAccumulationBuffer();
		processBuffer();
		newSelection = false;
	}

	juce::Path plot1;
	juce::Path plot2;
	juce::Path xAxis;
	juce::Path xAxisMarkers;
	juce::Path yAxis;
	juce::Path yAxisMarkersUp;
	juce::Path yAxisMarkersDown;
	juce::Path zeroTick;

	//** graph scaling variables **//
	float border_xBuffer = getWidth() * 0.295;
	float border_yBuffer = y_componentOffset;
	float widthBorder = getWidth() - x_componentOffset;
	float heightBorder = getHeight() - 240;
	float xBuffer = border_xBuffer + 2;
	float yBuffer = border_yBuffer + 12;
	float lengthXAxis = widthBorder;
	float lengthYAxis = heightBorder * .95;
	float yStartXYAxis = yBuffer + lengthYAxis - 1;
	float xStartXYAxis = xBuffer - 3;
	float yStartPlot = (yBuffer + lengthYAxis) / 2;
	float xAxisScale = 0.702;

		int zoom_xMax;

		//float xDiff = xMax - xMin;
		if (xDiff <= 0)  // handles divide by zero errors 
		{
			xMax = xMaxPrev;
			xMin = xMinPrev;
			xDiff = xMaxPrev - xMinPrev;
			inputXmin.setText(std::to_string(xMinPrev), juce::dontSendNotification);
			inputXmax.setText(std::to_string(xMaxPrev), juce::dontSendNotification);
		}
		else
		{
			xMaxPrev = xMax;
			xMinPrev = xMin;
		}
		//float scaleX = lengthXAxis / xDiff;  // Scaling X increments; pixels shown per sample
		//float xShift = -xMin * scaleX;

	int zoom_xMax;

	float xDiff = xMax - xMin;
	if (xDiff <= 0)  // handles divide by zero errors 
	{
		xMax = xMaxPrev;
		xMin = xMinPrev;
		xDiff = xMaxPrev - xMinPrev;
		inputXmin.setText(std::to_string(xMinPrev), juce::dontSendNotification);
		inputXmax.setText(std::to_string(xMaxPrev), juce::dontSendNotification);
	}
	else
	{
		xMaxPrev = xMax;
		xMinPrev = xMin;
	}
	float scaleX = lengthXAxis / xDiff;  // Scaling X increments; pixels shown per sample
	float xShift = -xMin * scaleX;

	float yDiff = yMax - yMin;
	if (yDiff <= 0)  // handles divide by zero errors
	{
		yMax = yMaxPrev;
		yMin = yMinPrev;
		yDiff = yMaxPrev - yMinPrev;
		inputYmin.setText(std::to_string(yMinPrev), juce::dontSendNotification);
		inputYmax.setText(std::to_string(yMaxPrev), juce::dontSendNotification);
	}
	else
	{
		yMaxPrev = yMax;
		yMinPrev = yMin;
	}
	float scaleY = -lengthYAxis / yDiff;  // Scaling Y increments; pixels shown per sample
	float yShift = (yDiff - 2.0f * yMax) * scaleY / 2.0f;

		//float plotYShift = yStartPlot + yShift;

		// Graph plots
		int logScale = 40;
		if (audioProcessor.minBlockSize) {
			if (setToLog == true) {
				xMax = std::log10(xMax);
				plot2.startNewSubPath(xStartXYAxis + xShift, yStartPlot + logScale * std::log10(binMag[1][0]) * scaleY + yShift);
				plot1.startNewSubPath(xStartXYAxis + xShift, yStartPlot + logScale * std::log10(binMag[0][0]) * scaleY + yShift);
				for (int i = 1; i < indexToFreqMap.size(); i++)
				{
					if (isVisiblePlot2 == true) {
						plot2.lineTo(std::log10(indexToFreqMap[i]) * xAxisScale * scaleX + xStartXYAxis + xShift, logScale * std::log10(binMag[1][i]) * scaleY + plotYShift);
					}
					if (isVisiblePlot1 == true) {
						plot1.lineTo(std::log10(indexToFreqMap[i]) * xAxisScale * scaleX + xStartXYAxis + xShift, logScale * std::log10(binMag[0][i]) * scaleY + plotYShift);
					}
				}
			}
			else {
				xMax = maxFreq / 5;
				plot2.startNewSubPath(xStartXYAxis + xShift, yStartPlot + logScale * std::log10(binMag[1][0]) * scaleY + yShift);
				plot1.startNewSubPath(xStartXYAxis + xShift, yStartPlot + logScale * std::log10(binMag[0][0]) * scaleY + yShift);
				for (int i = 1; i < indexToFreqMap.size(); i++)
				{
					if (isVisiblePlot2 == true) {
						plot2.lineTo(indexToFreqMap[i] * xAxisScale * scaleX + xStartXYAxis + xShift, logScale * std::log10(binMag[1][i]) * scaleY + plotYShift);
					}
					if (isVisiblePlot1 == true) {
						plot1.lineTo(indexToFreqMap[i] * xAxisScale * scaleX + xStartXYAxis + xShift, logScale * std::log10(binMag[0][i]) * scaleY + plotYShift);
					}
				}
			}

			g.setColour(juce::Colours::lightgreen);
			g.strokePath(plot2, juce::PathStrokeType(3.0f));
			g.setColour(juce::Colours::cornflowerblue);
			g.strokePath(plot1, juce::PathStrokeType(3.0f));
		}
		else {
			g.setColour(juce::Colours::black);
			g.fillRoundedRectangle(border_xBuffer, border_yBuffer, widthBorder, heightBorder, 3);
			g.setColour(juce::Colours::white);
			g.drawText("Not enough data selected", juce::Rectangle<int>(border_xBuffer, border_yBuffer, widthBorder, heightBorder), juce::Justification::centred, true);
		}
		initialWindow = false;
	}
	// Axis variables
	//int numXMarkers = zoom_xMax 
	int numXMarkers = xDiff;
	int numYMarkers = yDiff;

	// Plot X Axis Markers
	for (int i = 1; i <= numXMarkers; i++) {
		xAxisMarkers.startNewSubPath(xStartXYAxis + (i * xAxisScale * scaleX), yStartXYAxis - 5);
		xAxisMarkers.lineTo(xStartXYAxis + (i * xAxisScale * scaleX), yStartXYAxis + 5);
	}
	g.setColour(juce::Colours::white);
	g.strokePath(xAxisMarkers, juce::PathStrokeType(2.0f));

	// Plot Y Axis Markers
	for (int i = 1; i <= numYMarkers; i++) {
		yAxisMarkersUp.startNewSubPath(xStartXYAxis - 5, yStartPlot + (scaleY * i) + yShift);
		yAxisMarkersUp.lineTo(xStartXYAxis + 5, yStartPlot + (scaleY * i) + yShift);  // drawing line markers moving up from midpoint
		yAxisMarkersDown.startNewSubPath(xStartXYAxis - 5, yStartPlot - (scaleY * i) + yShift);
		yAxisMarkersDown.lineTo(xStartXYAxis + 5, yStartPlot - (scaleY * i) + yShift);  // drawing line markers moving up from midpoint
	}
	g.setColour(juce::Colours::white);
	g.strokePath(yAxisMarkersUp, juce::PathStrokeType(2.0f));
	g.strokePath(yAxisMarkersDown, juce::PathStrokeType(2.0f));

	//Plot zero on Y-axis
	zeroTick.startNewSubPath(xStartXYAxis - 15, yStartPlot + yShift);
	zeroTick.lineTo(xStartXYAxis + 15, yStartPlot + yShift);
	g.strokePath(zeroTick, juce::PathStrokeType(3.0f));

	//** draw graph border **//
	juce::Path graphBoundary;
	graphBoundary.startNewSubPath(border_xBuffer, border_yBuffer);
	graphBoundary.lineTo(widthBorder, border_yBuffer);
	graphBoundary.lineTo(widthBorder, heightBorder);
	graphBoundary.lineTo(border_xBuffer, heightBorder);
	graphBoundary.lineTo(border_xBuffer, border_yBuffer);
	g.setColour(juce::Colours::slategrey);
	g.strokePath(graphBoundary, juce::PathStrokeType(1.0f));

	//** draw boxes to hide out of bound plots **//
	int x_LeftBoxOffset = 0;
	int y_LeftBoxOffset = 0;
	int width_LeftBox = border_xBuffer;
	int height_LeftBox = getHeight();

	int x_TopBoxOffset = 0;
	int y_TopBoxOffset = 0;
	int width_TopBox = getWidth();
	int height_TopBox = border_yBuffer;

	int x_RightBoxOffset = widthBorder + 0.5;
	int y_RightBoxOffset = 0;
	int width_RightBox = getWidth();
	int height_RightBox = getHeight();

	int x_BottonBoxOffset = 0;
	int y_BottomBoxOffset = heightBorder;
	int width_BottomBox = getWidth();
	int height_BottomBox = getHeight();

	juce::Rectangle<int> leftPanel(x_LeftBoxOffset, y_LeftBoxOffset, width_LeftBox, height_LeftBox);
	juce::Rectangle<int> rightPanel(x_RightBoxOffset, y_RightBoxOffset, width_RightBox, height_RightBox);
	juce::Rectangle<int> topPanel(x_TopBoxOffset, y_TopBoxOffset, width_TopBox, height_TopBox);
	juce::Rectangle<int> bottomPanel(x_BottonBoxOffset, y_BottomBoxOffset, width_BottomBox, height_BottomBox);
	g.setColour(juce::Colours::black);
	g.fillRect(leftPanel);
	g.fillRect(rightPanel);
	g.fillRect(topPanel);
	g.fillRect(bottomPanel);

	// ** NEW STUFF ** //

	//** line to seperate left-side components and right-side components **//
	g.setColour(juce::Colours::darkgrey);
	g.fillRect(width_primaryCategoryLabel, 0, 1, windowMaxHeight);

	//** white area of plot selection in IMPORT AUDIO**//
	int yMargin_selectionBox = height_primaryCategoryLabel + yOffsetPrimary_secondaryLabel + height_secondaryLabel + yOffset_selectionBox;
	int xMargin_checkboxFill = 16;
	int yMargin_checkboxFill1 = 74;
	int yMargin_checkboxFill2 = 120;

	// draw white box
	g.setColour(juce::Colours::white);
	g.fillRoundedRectangle(x_componentOffset, yMargin_selectionBox, width_selectionBox, height_selectionBox, 3);

	// fill in checkboxes
	if (isVisiblePlot1 == true) {
		g.setColour(juce::Colours::dodgerblue);
		g.fillRoundedRectangle(xMargin_checkboxFill, yMargin_checkboxFill1, 16, 16, 4);
	}
	if (isVisiblePlot2 == true) {
		g.setColour(juce::Colours::dodgerblue);
		g.fillRoundedRectangle(xMargin_checkboxFill, yMargin_checkboxFill2, 16, 16, 4);
	}

	// draw line to seperate plot selections
	int xMargin_selectionBoundary = 2.5 * x_componentOffset;
	int yMargin_selectionBoundary = height_primaryCategoryLabel + yOffsetPrimary_secondaryLabel + height_secondaryLabel + (23 * yOffset_selectionBox);
	g.setColour(juce::Colours::lightgrey);
	g.fillRect(xMargin_selectionBoundary, yMargin_selectionBoundary, 243, 1);

	//** line to seperate upper and lower x/y bounds in ZOOM **//
	int xMargin_zoomBoundary = 2.5 * x_componentOffset;
	int yMargin_zoomBoundary = (119.5 * yOffset_selectionBox);
	g.setColour(juce::Colours::darkgrey);
	g.fillRect(xMargin_zoomBoundary, yMargin_zoomBoundary, 245, 1);

	//** white box for cursor label **//
	int xMargin_cursorBox = xStartXYAxis + 138;
	int yMargin_cursorBox = heightBorder + 55;
	int width_cursorBox = 180;
	int height_cursorBox = 26;
	// draw white box
	//g.setColour(juce::Colours::white);
	//g.fillRoundedRectangle(xMargin_cursorBox, yMargin_cursorBox, width_cursorBox, height_cursorBox, 2);

	//** white box for peak label **//
	int xMargin_peakBox = xMargin_cursorBox + 205;
	int yMargin_peakBox = yMargin_cursorBox;
	int width_peakBox = 180;
	int height_peakBox = 26;
	// draw white box
	//g.fillRoundedRectangle(xMargin_peakBox, yMargin_peakBox, width_peakBox, height_peakBox, 2);

	// ** END OF NEW STUFF ** //

	// Peak 
	float cursorYPeak = findPeak();
	if (cursorYPeak != 0) {
		g.setColour(juce::Colours::red);
		juce::Rectangle<int> peakLine(graphToScreen(cursorPeak), y_componentOffset, 1, lengthYAxis);
		g.fillRect(peakLine);
		g.setColour(juce::Colours::white);
		if (isVisiblePlot1)
			peakPlot.setText("(" + floatToStringPrecision((float)cursorPeak, 1) + ", " + floatToStringPrecision(cursorYPeak, 2) + ")", juce::dontSendNotification);
		if (isVisiblePlot2)
			peakPlot.setText("(" + floatToStringPrecision((float)cursorPeak, 1) + ", " + floatToStringPrecision(cursorYPeak, 2) + ")", juce::dontSendNotification);
	}
}

void FFTSpectrumAnalyzerAudioProcessorEditor::timerCallback()
{
	if (!isRunning && audioProcessor.getProcBlockCalled()) {
		isRunning = true;
		//audioProcessor.resetProcBlockCalled();
	}
	else if (isRunning && !audioProcessor.getProcBlockCalled()) {
		isRunning = false;
		newSelection = true;
		audioProcessor.setInitialBlock();
		repaint();
		//audioProcessor.resetScopeDataIndex();
	}
	audioProcessor.resetProcBlockCalled();
}

void FFTSpectrumAnalyzerAudioProcessorEditor::handleNewSelection(int numBins, int rowSize, int rowIndex)
{
	if (count == 0) {  //prepping all existing rows
		for (int r = 0; r < rowSize; r++) {
			audioProcessor.prepSelection(numBins, rowSize, r);
		}
	}

	else if (count > countPrev)
	{  //handling new row selection
		if (rowIndex > rowSize)
		{
			audioProcessor.prepSelection(numBins, rowSize, rowIndex);
		}
		else {
			return;
		}  //there is no selection made, return
	}
	countPrev = count;
	count++;
}

void FFTSpectrumAnalyzerAudioProcessorEditor::resized()
{
	// This is generally where you'll want to lay out the positions of any
	// subcomponents in your editor..

	//** graph scaling variables **//
	float border_xBuffer = getWidth() * 0.295;
	float border_yBuffer = y_componentOffset;
	float widthBorder = getWidth() - x_componentOffset;
	float heightBorder = getHeight() - 240;
	float xBuffer = border_xBuffer + 2;
	float yBuffer = border_yBuffer + 12;
	float lengthXAxis = widthBorder;
	float lengthYAxis = heightBorder * .95;
	float yStartXYAxis = yBuffer + lengthYAxis - 1;
	float xStartXYAxis = xBuffer - 3;
	float yStartPlot = (yBuffer + lengthYAxis) / 2;

	//** margins for primary labels **//
	int yMargin_selectTraceLabel = height_primaryCategoryLabel + yOffsetPrimary_secondaryLabel;
	int yMargin_zoomLabel = yMargin_selectTraceLabel + (22.5 * y_componentOffset);
	int yMargin_exportLabel = yMargin_selectTraceLabel + (42 * y_componentOffset);

	// secondary gui element width
	int width_toggleButton = 30;
	int width_plotLabel = 50;
	int width_selectButton = 90;
	int width_inputTextbox = 60;
	int width_exportButton = 95;
	int width_comboBox = 160;

	// secondary gui element height
	int heightControlWidget = 24;
	int height_toggleButton = heightControlWidget;
	int height_plotLabel = heightControlWidget;
	int height_selectButton = heightControlWidget;
	int height_inputTextbox = heightControlWidget - 2;
	int height_exportButton = heightControlWidget + 4;
	int height_comboBox = 30;

	//** margins for combo

	//** cursor *//
	// label
	int xMargin_cursorLabel = xStartXYAxis + 133;
	int yMargin_cursorLabel = heightBorder + 30;
	int xMargin_cursorPlot = xMargin_cursorLabel;
	int yMargin_cursorPlot = yMargin_cursorLabel + 22;


	//** peak **//
	// label
	int xMargin_peakLabel = xMargin_cursorLabel + 205;
	int yMargin_peaklabel = yMargin_cursorLabel;
	int xMargin_peakPlot = xMargin_peakLabel;
	int yMargin_peakPlot = yMargin_cursorPlot;


	//** window function **//
	//label
	int xMargin_windowLabel = xStartXYAxis + 65;
	int yMargin_windowLabel = yMargin_peaklabel + 62;
	// combobox
	int xMargin_winCombo = xMargin_windowLabel + 4;
	int yMargin_winCombo = yMargin_windowLabel + 22;

	//** axis **//
	// label
	int xMargin_axisLabel = xMargin_winCombo + 180;
	int yMargin_axisLabel = yMargin_windowLabel;
	// combobox
	int xMargin_axisCombo = xMargin_axisLabel + 4;
	int yMargin_axisCombo = yMargin_winCombo;


	//** size **//
	// label
	int xMargin_sizeLabel = xMargin_axisLabel + 180;
	int yMargin_sizeLabel = yMargin_axisLabel;
	// combobox
	int xMargin_sizeCombo = xMargin_sizeLabel + 4;
	int yMargin_sizeCombo = yMargin_winCombo;

	//** Set bounds for right side elements **//
	cursorLabel.setBounds(xMargin_cursorLabel, yMargin_cursorLabel, width_secondaryLabel, height_secondaryLabel);
	cursorPlot.setBounds(xMargin_cursorPlot, yMargin_cursorPlot, width_secondaryLabel, height_secondaryLabel);
	peakLabel.setBounds(xMargin_peakLabel, yMargin_peaklabel, width_secondaryLabel, height_secondaryLabel);
	peakPlot.setBounds(xMargin_peakPlot, yMargin_peakPlot, width_secondaryLabel, height_secondaryLabel);
	windowLabel.setBounds(xMargin_windowLabel, yMargin_windowLabel, width_secondaryLabel, height_secondaryLabel);
	axisLabel.setBounds(xMargin_axisLabel, yMargin_axisLabel, width_secondaryLabel, height_secondaryLabel);
	sizeLabel.setBounds(xMargin_sizeLabel, yMargin_sizeLabel, width_secondaryLabel, height_secondaryLabel);

	//cursorFunction.setBounds(xMargin_cursorCombo, yMargin_cursorCombo, width_comboBox, height_comboBox);
	//peakFunction.setBounds(xMargin_peakCombo, yMargin_peakCombo, width_comboBox, height_comboBox);
	windowFunction.setBounds(xMargin_winCombo, yMargin_winCombo, width_comboBox, height_comboBox);
	axis.setBounds(xMargin_axisCombo, yMargin_axisCombo, width_comboBox, height_comboBox);
	size.setBounds(xMargin_sizeCombo, yMargin_sizeCombo, width_comboBox, height_comboBox);


	//** plot 1 **//
	// toggle button 1
	int xMargin_toggleButton1 = 2 * x_componentOffset;
	int yMargin_toggleButton1 = height_primaryCategoryLabel + yOffsetPrimary_secondaryLabel + height_secondaryLabel + (6 * yOffset_selectionBox);
	// plot label 1
	int xMargin_plotLabel1 = 4 * xMargin_toggleButton1;
	int yMargin_plotLabel1 = yMargin_toggleButton1;
	// selection button 1
	int xMargin_selectButton1 = 3.5 * xMargin_plotLabel1;
	int yMargin_selectButton1 = yMargin_toggleButton1;

	//** plot 2 **//
	// toggle button 2
	int xMargin_toggleButton2 = xMargin_toggleButton1;
	int yMargin_toggleBotton2 = yMargin_toggleButton1 + (23 * yOffset_selectionBox);
	// plot label 2
	int xMargin_plotLabel2 = xMargin_plotLabel1;
	int yMargin_plotLabel2 = yMargin_toggleBotton2;
	// selection button 2
	int xMargin_selectButton2 = xMargin_selectButton1;
	int yMargin_selectButton2 = yMargin_toggleBotton2;

	//** upper bounds **//
	// upper label
	int yMargin_upperLabel = yMargin_zoomLabel + height_primaryCategoryLabel + yOffsetPrimary_secondaryLabel;

	// xMax input
	int xMargin_xMax = 10 * x_componentOffset;
	int yMargin_xMax = yMargin_upperLabel + 2;

	// x label
	int xMargin_xMaxLabel = 20.5 * x_componentOffset;
	int yMargin_xMaxLabel = yMargin_upperLabel;

	// yMax input
	int xMargin_yMax = 30 * x_componentOffset;
	int yMargin_yMax = yMargin_upperLabel + 2;

	// y label
	int xMargin_yMaxLabel = 40.5 * x_componentOffset;;
	int yMargin_yMaxLabel = yMargin_upperLabel;

	//** lower bounds **//
	// lower label
	int yMargin_lowerLabel = yMargin_upperLabel + (8 * y_componentOffset);

	// xMin input
	int xMargin_xMin = xMargin_xMax;
	int yMargin_xMin = yMargin_lowerLabel + 2;

	// x label
	int xMargin_xMinLabel = xMargin_xMaxLabel;
	int yMargin_xMinLabel = yMargin_lowerLabel;

	// yMin input
	int xMargin_yMin = xMargin_yMax;
	int yMargin_yMin = yMargin_lowerLabel + 2;

	// y label
	int xMargin_yMinLabel = xMargin_yMaxLabel;
	int yMargin_yMinLabel = yMargin_lowerLabel;

	//** export button **//
	int xMargin_exportButton = x_componentOffset;
	int yMargin_exportButton = yMargin_exportLabel + height_primaryCategoryLabel + (1.5 * yOffsetPrimary_secondaryLabel);

	//** set bounds for GUI elements **//
	gui_importAudio.setBounds(0, 0, width_primaryCategoryLabel, height_primaryCategoryLabel);
	gui_selectTrace.setBounds(0, yMargin_selectTraceLabel, width_secondaryLabel, height_secondaryLabel);
	gui_zoom.setBounds(0, yMargin_zoomLabel, width_primaryCategoryLabel, height_primaryCategoryLabel);
	gui_export.setBounds(0, yMargin_exportLabel, width_primaryCategoryLabel, height_primaryCategoryLabel);

	//** set bounds for secondary GUI elements **//
	toggleButtonPlot1.setBounds(xMargin_toggleButton1, yMargin_toggleButton1, width_toggleButton, height_toggleButton);
	toggleButtonPlot2.setBounds(xMargin_toggleButton2, yMargin_toggleBotton2, width_toggleButton, height_toggleButton);

	labelPlot1.setBounds(xMargin_plotLabel1, yMargin_plotLabel1, width_plotLabel, height_plotLabel);
	labelPlot2.setBounds(xMargin_plotLabel2, yMargin_plotLabel2, width_plotLabel, height_plotLabel);

	buttonPlot1.setBounds(xMargin_selectButton1, yMargin_selectButton1, width_selectButton, height_selectButton);
	buttonPlot2.setBounds(xMargin_selectButton2, yMargin_selectButton2, width_selectButton, height_selectButton);

	gui_upper.setBounds(0, yMargin_upperLabel, width_secondaryLabel, height_secondaryLabel);
	inputXmax.setBounds(xMargin_xMax, yMargin_xMax, width_inputTextbox, height_inputTextbox);
	gui_xUpper.setBounds(xMargin_xMaxLabel, yMargin_xMaxLabel, width_secondaryLabel, height_secondaryLabel);
	inputYmax.setBounds(xMargin_yMax, yMargin_yMax, width_inputTextbox, height_inputTextbox);
	gui_yUpper.setBounds(xMargin_yMaxLabel, yMargin_yMaxLabel, width_inputTextbox, height_inputTextbox);

	gui_lower.setBounds(0, yMargin_lowerLabel, width_secondaryLabel, height_secondaryLabel);
	inputXmin.setBounds(xMargin_xMin, yMargin_xMin, width_inputTextbox, height_inputTextbox);
	gui_xLower.setBounds(xMargin_xMinLabel, yMargin_xMinLabel, width_secondaryLabel, height_secondaryLabel);
	inputYmin.setBounds(xMargin_yMax, yMargin_yMin, width_inputTextbox, height_inputTextbox);
	gui_yLower.setBounds(xMargin_yMinLabel, yMargin_yMinLabel, width_secondaryLabel, height_secondaryLabel);

	gui_exportButton.setBounds(xMargin_exportButton, yMargin_exportButton, width_exportButton, height_exportButton);
}

void FFTSpectrumAnalyzerAudioProcessorEditor::setFreqData(int fftData) {
	numBins = fftSize / 2 + 1;
	numFreqBins = fftSize / 2;
	stepSize = fftSize / 2;
	//indexToFreqMap.resize(numBins);
	binMag.resize(rowSize, std::vector<float>(numBins, 0));
}

void FFTSpectrumAnalyzerAudioProcessorEditor::getBounds()
{
	float minVal = -1000;
	float maxVal = 24000;
	juce::String temp = inputXmin.getText(false);
	float val = std::atof(temp.toStdString().c_str());
	if (val >= minVal && val <= maxVal)
	{
		xMin = val;
	}
	else { inputXmin.setText(std::to_string(xMin), juce::dontSendNotification); }

	temp = inputXmax.getText(false);
	val = std::atof(temp.toStdString().c_str());
	if (val >= minVal && val <= maxVal)
	{
		xMax = val;
	}
	else { inputXmax.setText(std::to_string(xMax), juce::dontSendNotification); }

	temp = inputYmin.getText(false);
	val = std::atof(temp.toStdString().c_str());
	if (val >= minVal && val <= maxVal)
	{
		yMin = val;
	}
	else { inputYmin.setText(std::to_string(yMin), juce::dontSendNotification); }

	temp = inputYmax.getText(false);
	val = std::atof(temp.toStdString().c_str());
	if (val >= minVal && val <= maxVal)
	{
		yMax = val;
	}
	else { inputYmax.setText(std::to_string(yMax), juce::dontSendNotification); }
	repaint();
}


void FFTSpectrumAnalyzerAudioProcessorEditor::setPlotIndex(int plotIndex)
{
	rowIndex = plotIndex;
	audioProcessor.setRowIndex(rowIndex);
	if (plotIndex == 0)
	{
		buttonPlot1.setButtonText("Selected");
		buttonPlot2.setButtonText("Select");
	}
	else if (plotIndex == 1)
	{
		buttonPlot2.setButtonText("Selected");
		buttonPlot1.setButtonText("Select");
	}
}

void FFTSpectrumAnalyzerAudioProcessorEditor::updateToggleState(int plotId)
{
	if (plotId == 1)
	{
		setVisibility(1);
	}
	else
		setVisibility(2);
	repaint();
}


void FFTSpectrumAnalyzerAudioProcessorEditor::setVisibility(int plotId)
{
	if (plotId == 1)
	{
		if (isVisiblePlot1 == false) {
			isVisiblePlot1 = true;
		}
		else if (isVisiblePlot1 == true) {
			isVisiblePlot1 = false;
		}
	}
	else
	{
		if (isVisiblePlot2 == false) {
			isVisiblePlot2 = true;
		}
		else if (isVisiblePlot2 == true) {
			isVisiblePlot2 = false;
		}
	}
}

void FFTSpectrumAnalyzerAudioProcessorEditor::setWindowFunction() {
	juce::dsp::WindowingFunction<float>::WindowingMethod newWindow;
	switch (windowFunction.getSelectedId()) {
	case 1:
		newWindow = juce::dsp::WindowingFunction<float>::WindowingMethod::blackman;
		setWindow(newWindow);
		repaint();
		break;
	case 2:
		newWindow = juce::dsp::WindowingFunction<float>::WindowingMethod::blackmanHarris;
		setWindow(newWindow);
		repaint();
		break;
	case 3:
		newWindow = juce::dsp::WindowingFunction<float>::WindowingMethod::flatTop;
		setWindow(newWindow);
		repaint();
		break;
	case 4:
		newWindow = juce::dsp::WindowingFunction<float>::WindowingMethod::hamming;
		setWindow(newWindow);
		repaint();
		break;
	case 5:
		newWindow = juce::dsp::WindowingFunction<float>::WindowingMethod::hann;
		setWindow(newWindow);
		repaint();
		break;
	case 6:
		newWindow = juce::dsp::WindowingFunction<float>::WindowingMethod::kaiser;
		setWindow(newWindow);
		repaint();
		break;
	case 7:
		newWindow = juce::dsp::WindowingFunction<float>::WindowingMethod::rectangular;
		setWindow(newWindow);
		repaint();
		break;
	case 8:
		newWindow = juce::dsp::WindowingFunction<float>::WindowingMethod::triangular;
		setWindow(newWindow);
		repaint();
		break;
	}

}

void FFTSpectrumAnalyzerAudioProcessorEditor::setBlockSize() {
	auto selection = size.getText();
	fftSize = selection.getIntValue();
	setFreqData(fftSize);
	//audioProcessor.setInitialAccBuffer();
	//processBuffer();
	repaint();
}

void FFTSpectrumAnalyzerAudioProcessorEditor::setAxisType() {
	if (setToLog == true) {
		setToLog = false;
		repaint();
	}
	else {
		setToLog = true;
		repaint();
	}
}

void FFTSpectrumAnalyzerAudioProcessorEditor::setWindow(juce::dsp::WindowingFunction<float>::WindowingMethod type) {
	juce::dsp::WindowingFunction<float> window(fftSize, type);
	window.fillWindowingTables(fftSize, type);
}

void FFTSpectrumAnalyzerAudioProcessorEditor::processBuffer() {

	zeroBuffers();

	int bufferShift = 0;

	juce::dsp::FFT forwardFFT(std::log2(fftSize));

	int sampleRate = audioProcessor.getBlockSampleRate();

	maxFreq = sampleRate / 2;
	//x variable for labeling
	for (int i = 0; i < numBins; i++) {
		indexToFreqMap[i] = i * ((float)maxFreq / (float)numFreqBins);
	}

	while(bufferShift <=sampleSelections[rowIndex].size()-stepSize){
	//while (buffSize >= numFreqBins) {

		std::copy(bufferLeft.begin(), bufferLeft.begin() + stepSize, bufferLeft.begin() + stepSize);
		std::copy(bufferRight.begin() + stepSize, bufferRight.end(), bufferLeft.begin());
		std::copy(bufferRight.begin(), bufferRight.begin() + stepSize, bufferRight.begin() + stepSize);

		std::copy(sampleSelections[rowIndex].begin()+ bufferShift,sampleSelections[rowIndex].begin()+(bufferShift+ stepSize),bufferRight.begin());
		//buffer.read(bufferRight.data(), numFreqBins);
		std::copy(bufferRight.begin(), bufferRight.end(), windowBufferRight.begin());
		windowBufferLeft = bufferLeft;
		editWindow.multiplyWithWindowingTable(windowBufferRight.data(), fftSize);
		editWindow.multiplyWithWindowingTable(windowBufferLeft.data(), fftSize);
		forwardFFT.performRealOnlyForwardTransform(windowBufferRight.data(), true);
		fftCounter++;

		for (int i = 0; i < numBins; i++) {
			binMag[rowIndex][i] += sqrt(pow(windowBufferRight[2 * i], 2) + pow(windowBufferRight[2 * i + 1], 2)) / numFreqBins;
		}
		bufferShift += numFreqBins;
	}
	if (fftCounter != 0)
	{
		for (int i = 0; i < numBins; i++) {
			binMag[rowIndex][i] /= fftCounter;
		}
	}
}

void FFTSpectrumAnalyzerAudioProcessorEditor::zeroBuffers() {
	bufferLeft.resize(fftSize);
	std::fill(bufferLeft.begin(), bufferLeft.end(), 0.0f);
	bufferRight.resize(fftSize);
	std::fill(bufferRight.begin(), bufferRight.end(), 0.0f);
	windowBufferRight.resize(fftSize * 2);
	std::fill(windowBufferRight.begin(), windowBufferRight.end(), 0.0f);
	windowBufferLeft.resize(fftSize);
	std::fill(windowBufferLeft.begin(), windowBufferLeft.end(), 0.0f);
	binMag[rowIndex].resize(numBins);
	std::fill(binMag[rowIndex].begin(), binMag[rowIndex].end(), 0.0f);
	indexToFreqMap.resize(numBins);
	std::fill(indexToFreqMap.begin(), indexToFreqMap.end(), 0.0f);
	fftCounter = 0;
}


void FFTSpectrumAnalyzerAudioProcessorEditor::mouseMove(const juce::MouseEvent& event)
{
	float graphWest = (getWidth() * 0.295) - 1;
	float graphNorth = y_componentOffset;
	float graphEast = (getWidth() - x_componentOffset) + 0.5;
	float graphSouth = (getHeight() - 240) * 0.95;

	cursorX = event.getMouseDownX();
	float cursorY = event.getMouseDownY();
	//invalid bounds
	if (cursorX < graphWest || cursorX > graphEast || cursorY < graphNorth || cursorY > graphSouth || screenToGraph(cursorX) < 1) {
		isGraph = false;
		cursorPlot.setText("(0.0, 0.00)", juce::dontSendNotification);
	}
	//valid bounds
	else {
		isGraph = true;
		cursorX = screenToGraph(cursorX);
		if (plotIndexSelection == 1 && isVisiblePlot2) {
			cursorPlot.setText("(" + floatToStringPrecision(cursorX, 1) + ", " + floatToStringPrecision(getYCoord(1, setToLog, (int)cursorX), 2) + ")", juce::dontSendNotification);
		}
		if (plotIndexSelection == 0 && isVisiblePlot1) {
			cursorPlot.setText("(" + floatToStringPrecision(cursorX, 1) + ", " + floatToStringPrecision(getYCoord(0, setToLog, (int)cursorX), 2) + ")", juce::dontSendNotification);
		}
	}
	repaint();
}

float FFTSpectrumAnalyzerAudioProcessorEditor::findPeak()
{
	int p = plotIndexSelection;

	float maxValue = getYCoord(p, setToLog, 0);
	for (size_t i = 1; i < binMag[p].size(); ++i) {
		if (getYCoord(p, setToLog, i) > maxValue) {
			maxValue = getYCoord(p, setToLog, i);
			cursorPeak = i;
		}
	}
	//rob sucks
	//rian swallows
	return maxValue;
}

float FFTSpectrumAnalyzerAudioProcessorEditor::getYCoord(int plotNumber, bool log, int index) {
	//if (log) {
	return 40 * std::log10(binMag[plotNumber][index]);
	//}
	//else {
	//	return binMag[plotNumber][index];
	//}
}

float FFTSpectrumAnalyzerAudioProcessorEditor::screenToGraph(float screenCoord) {
	float graphWest = (getWidth() * 0.295) - 1;
	float graphNorth = y_componentOffset;
	float graphEast = (getWidth() - x_componentOffset) + 0.5;
	float graphSouth = (getHeight() - 240) * 0.95;

	screenCoord -= graphWest;
	screenCoord = (screenCoord * (100) )/ (graphEast - graphWest);
	return screenCoord;
}

float FFTSpectrumAnalyzerAudioProcessorEditor::graphToScreen(int graphCoord) {
	float graphWest = (getWidth() * 0.295) - 1;
	float graphNorth = y_componentOffset;
	float graphEast = (getWidth() - x_componentOffset) + 0.5;
	float graphSouth = (getHeight() - 240) * 0.95;

	graphCoord = (graphCoord * (graphEast - graphWest)) / (100);
	graphCoord += graphWest;
	return graphCoord;
}

std::string FFTSpectrumAnalyzerAudioProcessorEditor::floatToStringPrecision(float f, int p)
{
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(p) << f;
	return oss.str();
}