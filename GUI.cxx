#include "GUI.h"

#include <QMenu>
#include <QFileDialog>
#include <QDir>
#include <QInputDialog>

#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkConeSource.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkInteractorStyle.h"
#include "vtkTDxInteractorStyleCamera.h"
#include "vtkTDxInteractorStyleSettings.h"
#include "QVTKInteractor.h"

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkStructuredPoints.h>
#include <vtkStructuredPointsReader.h>
#include <vtkVolumeRayCastCompositeFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkVolumeRayCastMapper.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolumeProperty.h>
#include <vtkAxesActor.h>
#include <vtkImageShiftScale.h>
#include <vtkImageCast.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkPNGReader.h>

#include <vtkVolumeTextureMapper2D.h>

#include <vtkVolumeTextureMapper3D.h>

#include <vtkMarchingCubes.h>
#include <vtkProperty.h>

#include <vtkStripper.h>
#include "vtkMarchingCubesSpace.h"
#include <vtkSmoothPolyDataFilter.h>

GUI::GUI()
{
	this->setupUi(this);

	// create a window to make it stereo capable and give it to QVTKWidget
	vtkRenderWindow* renwin = vtkRenderWindow::New();
	renwin->StereoCapableWindowOn();

	// Activate 3DConnexion device only on the left render window.
	qVTK1->SetUseTDx(true);

	qVTK1->SetRenderWindow(renwin);
	renwin->Delete();

	const double angleSensitivity=0.02;
	const double translationSensitivity=0.001;

	QVTKInteractor *iren=qVTK1->GetInteractor();
	vtkInteractorStyle *s=
		static_cast<vtkInteractorStyle *>(iren->GetInteractorStyle());
	vtkTDxInteractorStyleCamera *t=
		static_cast<vtkTDxInteractorStyleCamera *>(s->GetTDxStyle());

	t->GetSettings()->SetAngleSensitivity(angleSensitivity);
	t->GetSettings()->SetTranslationXSensitivity(translationSensitivity);
	t->GetSettings()->SetTranslationYSensitivity(translationSensitivity);
	t->GetSettings()->SetTranslationZSensitivity(translationSensitivity);



	// add a renderer
	Ren1 = vtkRenderer::New();Ren1->SetBackground(1,1,1);
	qVTK1->GetRenderWindow()->AddRenderer(Ren1);

	// add a popup menu for the window and connect it to our slot
	QMenu* popup1 = new QMenu(qVTK1);
	popup1->addAction("Background White");
	popup1->addAction("Background Black");
	popup1->addAction("Stereo Rendering");
	connect(popup1, SIGNAL(triggered(QAction*)), this, SLOT(color1(QAction*)));



	// create a window to make it stereo capable and give it to QVTKWidget
	renwin = vtkRenderWindow::New();
	renwin->StereoCapableWindowOn();

	qVTK2->SetUseTDx(true);
	qVTK2->SetRenderWindow(renwin);
	renwin->Delete();

	QVTKInteractor *iren2=qVTK2->GetInteractor();
	vtkInteractorStyle *s2=
		static_cast<vtkInteractorStyle *>(iren2->GetInteractorStyle());
	vtkTDxInteractorStyle *t2=s2->GetTDxStyle();
	t2->SetSettings(t->GetSettings());

	// add a renderer
	Ren2 = vtkRenderer::New();Ren2->SetBackground(1,1,1);
	qVTK2->GetRenderWindow()->AddRenderer(Ren2);

	// add a popup menu for the window and connect it to our slot
	QMenu* popup2 = new QMenu(qVTK2);
	popup2->addAction("Background White");
	popup2->addAction("Background Black");
	popup2->addAction("Stereo Rendering");
	connect(popup2, SIGNAL(triggered(QAction*)), this, SLOT(color2(QAction*)));




	Connections = vtkEventQtSlotConnect::New();

	// get right mouse pressed with high priority
	Connections->Connect(qVTK1->GetRenderWindow()->GetInteractor(),
		vtkCommand::RightButtonPressEvent,
		this,
		SLOT(popup( vtkObject*, unsigned long, void*, void*, vtkCommand*)),
		popup1, 1.0);

	// get right mouse pressed with high priority
	Connections->Connect(qVTK2->GetRenderWindow()->GetInteractor(),
		vtkCommand::RightButtonPressEvent,
		this,
		SLOT(popup( vtkObject*, unsigned long, void*, void*, vtkCommand*)),
		popup2, 1.0);

	// connect window enter event to radio button slot
	Connections->Connect(qVTK1->GetRenderWindow()->GetInteractor(),
		vtkCommand::EnterEvent,
		radio1,
		SLOT(animateClick()));

	// connect window enter event to radio button slot
	Connections->Connect(qVTK2->GetRenderWindow()->GetInteractor(),
		vtkCommand::EnterEvent,
		radio2,
		SLOT(animateClick()));

	// update coords as we move through the window
	Connections->Connect(qVTK1->GetRenderWindow()->GetInteractor(),
		vtkCommand::MouseMoveEvent,
		this,
		SLOT(updateCoords(vtkObject*)));

	// update coords as we move through the window
	Connections->Connect(qVTK2->GetRenderWindow()->GetInteractor(),
		vtkCommand::MouseMoveEvent,
		this,
		SLOT(updateCoords(vtkObject*)));

	Connections->PrintSelf(cout, vtkIndent());
}

GUI::~GUI()
{
	Ren1->Delete();
	Ren2->Delete();

	Connections->Delete();
}


void GUI::updateCoords(vtkObject* obj)
{
	// get interactor
	vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
	// get event position
	int event_pos[2];
	iren->GetEventPosition(event_pos);
	// update label
	QString str;
	str.sprintf("x=%d : y=%d", event_pos[0], event_pos[1]);
	coord->setText(str);
}

void GUI::popup(vtkObject * obj, unsigned long,
				void * client_data, void *,
				vtkCommand * command)
{
	// A note about context menus in Qt and the QVTKWidget
	// You may find it easy to just do context menus on right button up,
	// due to the event proxy mechanism in place.

	// That usually works, except in some cases.
	// One case is where you capture context menu events that
	// child windows don't process.  You could end up with a second
	// context menu after the first one.

	// See QVTKWidget::ContextMenuEvent enum which was added after the
	// writing of this example.

	// get interactor
	vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
	// consume event so the interactor style doesn't get it
	command->AbortFlagOn();
	// get popup menu
	QMenu* popupMenu = static_cast<QMenu*>(client_data);
	// get event location
	int* sz = iren->GetSize();
	int* position = iren->GetEventPosition();
	// remember to flip y
	QPoint pt = QPoint(position[0], sz[1]-position[1]);
	// map to global
	QPoint global_pt = popupMenu->parentWidget()->mapToGlobal(pt);
	// show popup menu at global point
	popupMenu->popup(global_pt);
}

void GUI::color1(QAction* color)
{
	if(color->text() == "Background White")
		Ren1->SetBackground(1,1,1);
	else if(color->text() == "Background Black")
		Ren1->SetBackground(0,0,0);
	else if(color->text() == "Stereo Rendering")
	{
		Ren1->GetRenderWindow()->SetStereoRender(!Ren1->GetRenderWindow()->GetStereoRender());
	}
	qVTK1->update();
}

void GUI::color2(QAction* color)
{
	if(color->text() == "Background White")
		this->Ren2->SetBackground(1,1,1);
	else if(color->text() == "Background Black")
		this->Ren2->SetBackground(0,0,0);
	else if(color->text() == "Stereo Rendering")
	{
		this->Ren2->GetRenderWindow()->SetStereoRender(!this->Ren2->GetRenderWindow()->GetStereoRender());
	}
	qVTK2->update();
}


void GUI::on_actionInput_image_size_triggered()
{
	ImageLength=QInputDialog::getInt(NULL,tr(" "),tr("please input the length of each image:"));
	ImageWidth=QInputDialog::getInt(NULL,tr(" "),tr("please input the width of each image:"));    
}

void GUI::on_actionInput_the_number_of_image_series_triggered()
{
    NumberOfImages=QInputDialog::getInt(NULL,tr(" "),tr("please input the number of image series:"));
}

void GUI::on_actionInput_data_space_triggered()
{
	XDataSpace=QInputDialog::getInt(NULL,tr(" "),tr("please input the X axis data space:"));
	YDataSpace=QInputDialog::getInt(NULL,tr(" "),tr("please input the Y axis data space:")); 
	ZDataSpace=QInputDialog::getInt(NULL,tr(" "),tr("please input the Z axis data space:")); 
}

void GUI::on_actionRayCasting_triggered()
{
	 QDir dir;
	 QString fileName = QFileDialog::getExistingDirectory( this, QString(tr("open folder(only png image series can be read)")), dir.absolutePath() );
     if ( fileName.isEmpty() == true ) return;
	 fileName+="/";
	  // ֧�ִ�����·���Ķ�ȡ
      QByteArray ba = fileName.toLocal8Bit();
      const char *FilePrefix = ba.data();

	// �½��ļ���ȡ���󣬳�������vtkBMPReader��vtkDICOMImageReader��vtkJPEGReader��
	vtkSmartPointer<vtkPNGReader> pngReader =vtkSmartPointer<vtkPNGReader>::New();
	// ��ͬ��reader��Ҫ���õĲ����ǲ�ͬ�� ��˱������ʺ�jpegreader
	//pngReader->SetFilePrefix("G:/GraduationDesignNeeded/ExperimentalData/Interpolation/BinaryImage/P01/"); // binary image Ҫ�򿪵�·��
	//pngReader->SetFilePrefix("G:/GraduationDesignNeeded/ExperimentalData/Interpolation/Image/P01/"); // Ҫ�򿪵�·��
	pngReader->SetFilePrefix(FilePrefix);
	pngReader->SetFilePattern("%s%03d.png");// ͼƬ�ļ�����ʽ���˴�Ϊ 001.png 015.png ...
	pngReader->SetDataByteOrderToLittleEndian();
	pngReader->SetDataSpacing(XDataSpace, YDataSpace, ZDataSpace);  // ����ͼƬ�����رȣ������ò������������ٶ�֮ 
	pngReader->SetFileNameSliceSpacing(1);
	//pngReader->SetDataExtent(0, 109, 0, 109, 1, 29);
	// ������Ϊ��P01�ļ���������0.png ~ 29.png����������Ϊ 0��29
	// ÿ��ͼƬ�ĳ���Ϊ110 * 110 �������Ϊ0��109
	pngReader->SetDataExtent(0, ImageLength-1, 0, ImageWidth-1, 0, NumberOfImages-1);
	pngReader->Update(); 

	vtkSmartPointer<vtkVolumeRayCastCompositeFunction> rayCastFun = 
		vtkSmartPointer<vtkVolumeRayCastCompositeFunction>::New();

	vtkSmartPointer<vtkVolumeRayCastMapper> volumeMapper = 
		vtkSmartPointer<vtkVolumeRayCastMapper>::New();
	volumeMapper->SetInputData(pngReader->GetOutput());
	volumeMapper->SetVolumeRayCastFunction(rayCastFun);

	//���ù��߲�������
	//volumeMapper->SetSampleDistance(volumeMapper->GetSampleDistance()*4);
	//����ͼ���������
	//volumeMapper->SetAutoAdjustSampleDistances(0);
	//volumeMapper->SetImageSampleDistance(4);

	vtkSmartPointer<vtkVolumeProperty> volumeProperty = 
		vtkSmartPointer<vtkVolumeProperty>::New();
	volumeProperty->SetInterpolationTypeToLinear();
	volumeProperty->SetAmbient(0.4);
	volumeProperty->SetDiffuse(0.6);
	volumeProperty->SetSpecular(0.2);


	//��͸����ӳ�亯�������ù��߷����ϵĻҶ�ֵ���䲻͸����ӳ��

	vtkSmartPointer<vtkPiecewiseFunction> compositeOpacity = 
		vtkSmartPointer<vtkPiecewiseFunction>::New();
	compositeOpacity->AddPoint(0, 0.0);//�Ҷ�ֵ����͸����ֵ
	compositeOpacity->AddPoint(250, 1.00);
	compositeOpacity->AddPoint(600, 1.0);
	compositeOpacity->AddPoint(2000, 1.00);    //��͸����ֵΪ1��Ϊ��ȫ��͸��
	/*compositeOpacity->AddPoint(0, 1.0);
	compositeOpacity->AddPoint(255, 0.0);*/  //binary image
	volumeProperty->SetScalarOpacity(compositeOpacity); //���ò�͸���ȴ��亯��

	//��ɫӳ�亯�������ûҶ�ֵ��RGB��ɫ��ӳ��
	vtkSmartPointer<vtkColorTransferFunction> color = 
		vtkSmartPointer<vtkColorTransferFunction>::New();
	color->AddRGBPoint(0,    1.00, 1.00, 1.00);//�Ҷ�ֵ��RGB��ɫֵ 
	color->AddRGBPoint(200,  1.00, 0, 0.00);
	color->AddRGBPoint(600, 1.00, 1.00, 1.00);
	color->AddRGBPoint(2000, 1.00, 1.00, 1.00);
	/*color->AddRGBPoint(0,    1.00, 0.00, 0.00);
	color->AddRGBPoint(255,    1.00, 1.00, 1.00);*/  //binary image
	volumeProperty->SetColor(color);

	Ren1->RemoveVolume(volume);

	volume =vtkSmartPointer<vtkVolume>::New();
	volume->SetMapper(volumeMapper);
	volume->SetProperty(volumeProperty);

	Ren1->AddVolume( volume ); 
	Ren1->ResetCamera();

}


void GUI::on_actionTextureMapper2D_triggered()
{
	 QDir dir;
	 QString fileName = QFileDialog::getExistingDirectory( this, QString(tr("open folder(only png image series can be read)")), dir.absolutePath() );
     if ( fileName.isEmpty() == true ) return;
	 fileName+="/";
	  // ֧�ִ�����·���Ķ�ȡ
      QByteArray ba = fileName.toLocal8Bit();
      const char *FilePrefix = ba.data();

	// �½��ļ���ȡ���󣬳�������vtkBMPReader��vtkDICOMImageReader��vtkJPEGReader��
	vtkSmartPointer<vtkPNGReader> pngReader =vtkSmartPointer<vtkPNGReader>::New();
    // ��ͬ��reader��Ҫ���õĲ����ǲ�ͬ�� ��˱������ʺ�jpegreader
	//pngReader->SetFilePrefix("G:/GraduationDesignNeeded/ExperimentalData/Interpolation/BinaryImage/P01/"); // binary image Ҫ�򿪵�·��
   // pngReader->SetFilePrefix("G:/GraduationDesignNeeded/ExperimentalData/Interpolation/Image/P01/"); // Ҫ�򿪵�·��	 
	pngReader->SetFilePrefix(FilePrefix);
	pngReader->SetFilePattern("%s%03d.png");// ͼƬ�ļ�����ʽ���˴�Ϊ 001.png 015.png ...
	pngReader->SetDataByteOrderToLittleEndian();
	pngReader->SetDataSpacing(XDataSpace, YDataSpace, ZDataSpace);  // ����ͼƬ�����رȣ������ò������������ٶ�֮ 
	pngReader->SetFileNameSliceSpacing(1);
	//pngReader->SetDataExtent(0, 109, 0, 109, 1, 29);
	// ������Ϊ��P01�ļ���������0.png ~ 29.png����������Ϊ 0��29
	// ÿ��ͼƬ�ĳ���Ϊ110 * 110 �������Ϊ0��109
	pngReader->SetDataExtent(0, ImageLength-1, 0, ImageWidth-1, 0, NumberOfImages-1);
	pngReader->Update(); 

	vtkSmartPointer<vtkVolumeTextureMapper2D> volumeMapper = 
		vtkSmartPointer<vtkVolumeTextureMapper2D>::New();
	volumeMapper->SetInputData(pngReader->GetOutput());

	vtkSmartPointer<vtkVolumeProperty> volumeProperty = 
		vtkSmartPointer<vtkVolumeProperty>::New();
	volumeProperty->SetInterpolationTypeToLinear();
	volumeProperty->ShadeOn();
	volumeProperty->SetAmbient(0.4);
	volumeProperty->SetDiffuse(0.6);
	volumeProperty->SetSpecular(0.2);

	vtkSmartPointer<vtkPiecewiseFunction> compositeOpacity = 
		vtkSmartPointer<vtkPiecewiseFunction>::New();
	compositeOpacity->AddPoint(0, 0.0);//�Ҷ�ֵ����͸����ֵ
	compositeOpacity->AddPoint(100, 1.00);
	compositeOpacity->AddPoint(600, 1.0);
	compositeOpacity->AddPoint(2000, 1.00);    //��͸����ֵΪ1��Ϊ��ȫ��͸��
	/*compositeOpacity->AddPoint(0, 1.0);
	compositeOpacity->AddPoint(255, 0.0);*/  //binary image
	volumeProperty->SetScalarOpacity(compositeOpacity);

	vtkSmartPointer<vtkColorTransferFunction> color = 
		vtkSmartPointer<vtkColorTransferFunction>::New();
	color->AddRGBPoint(0,    1.00, 1.00, 1.00);//�Ҷ�ֵ��RGB��ɫֵ 
	color->AddRGBPoint(200,  1.00, 0, 0.00);
	color->AddRGBPoint(600, 1.00, 1.00, 1.00);
	color->AddRGBPoint(2000, 1.00, 1.00, 1.00);
	/*color->AddRGBPoint(0,    1.00, 0.00, 0.00);
	color->AddRGBPoint(255,    1.00, 1.00, 1.00);*/  //binary image
	volumeProperty->SetColor(color);

	Ren1->RemoveVolume(volume);

	volume =vtkSmartPointer<vtkVolume>::New();
	volume->SetMapper(volumeMapper);
	volume->SetProperty(volumeProperty);

	Ren1->AddVolume( volume ); 
	Ren1->ResetCamera();

}


void GUI::on_actionTextureMapper3D_triggered()
{

	QDir dir;
	QString fileName = QFileDialog::getExistingDirectory( this, QString(tr("open folder(only png image series can be read)")), dir.absolutePath() );
	if ( fileName.isEmpty() == true ) return;
	fileName+="/";
	// ֧�ִ�����·���Ķ�ȡ
	QByteArray ba = fileName.toLocal8Bit();
	const char *FilePrefix = ba.data();

	// �½��ļ���ȡ���󣬳�������vtkBMPReader��vtkDICOMImageReader��vtkJPEGReader��
	vtkSmartPointer<vtkPNGReader> pngReader =vtkSmartPointer<vtkPNGReader>::New();
	// ��ͬ��reader��Ҫ���õĲ����ǲ�ͬ�� ��˱������ʺ�jpegreader
	//pngReader->SetFilePrefix("G:/GraduationDesignNeeded/ExperimentalData/Interpolation/BinaryImage/P01/"); // binary image Ҫ�򿪵�·��
	//pngReader->SetFilePrefix("G:/GraduationDesignNeeded/ExperimentalData/Interpolation/Image/P01/"); // Ҫ�򿪵�·��
	pngReader->SetFilePrefix(FilePrefix);
	pngReader->SetFilePattern("%s%03d.png");// ͼƬ�ļ�����ʽ���˴�Ϊ 001.png 015.png ...
	pngReader->SetDataByteOrderToLittleEndian();
	pngReader->SetDataSpacing(XDataSpace, YDataSpace, ZDataSpace);  // ����ͼƬ�����رȣ������ò������������ٶ�֮ 
	pngReader->SetFileNameSliceSpacing(1);
	//pngReader->SetDataExtent(0, 109, 0, 109, 1, 29);
	// ������Ϊ��P01�ļ���������0.png ~ 15.png����������Ϊ 0��29
	// ÿ��ͼƬ�ĳ���Ϊ110 * 110 �������Ϊ0��109
	pngReader->SetDataExtent(0, ImageLength-1, 0, ImageWidth-1, 0, NumberOfImages-1);
	pngReader->Update(); 

	vtkSmartPointer<vtkVolumeTextureMapper3D> volumeMapper = 
		vtkSmartPointer<vtkVolumeTextureMapper3D>::New();
	volumeMapper->SetInputData(pngReader->GetOutput());

	vtkSmartPointer<vtkVolumeProperty> volumeProperty = 
		vtkSmartPointer<vtkVolumeProperty>::New();
	volumeProperty->SetInterpolationTypeToLinear();
	//volumeProperty->ShadeOn();
	volumeProperty->SetAmbient(0.4);
	volumeProperty->SetDiffuse(0.6);
	volumeProperty->SetSpecular(0.2);

	vtkSmartPointer<vtkPiecewiseFunction> compositeOpacity = 
		vtkSmartPointer<vtkPiecewiseFunction>::New();
	compositeOpacity->AddPoint(0, 0.0);//�Ҷ�ֵ����͸����ֵ
	compositeOpacity->AddPoint(100, 1.00);
	compositeOpacity->AddPoint(600, 1.0);
	compositeOpacity->AddPoint(2000, 1.00);    //��͸����ֵΪ1��Ϊ��ȫ��͸��
	/*compositeOpacity->AddPoint(0, 1.0);
	compositeOpacity->AddPoint(255, 0.0);*/  //binary image
	volumeProperty->SetScalarOpacity(compositeOpacity);

	vtkSmartPointer<vtkColorTransferFunction> color = 
		vtkSmartPointer<vtkColorTransferFunction>::New();
	color->AddRGBPoint(0,    1.00, 1.00, 1.00);//�Ҷ�ֵ��RGB��ɫֵ 
	color->AddRGBPoint(200,  1.00, 0, 0.00);
	color->AddRGBPoint(600, 1.00, 1.00, 1.00);
	color->AddRGBPoint(2000, 1.00, 1.00, 1.00);
	/*color->AddRGBPoint(0,    1.00, 0.00, 0.00);
	color->AddRGBPoint(255,    1.00, 1.00, 1.00);*/  //binary image
	volumeProperty->SetColor(color);

	Ren1->RemoveVolume(volume);

	volume = vtkSmartPointer<vtkVolume>::New();
	volume->SetMapper(volumeMapper);
	volume->SetProperty(volumeProperty);

	Ren1->AddVolume( volume ); 
	Ren1->ResetCamera();

}


void GUI::on_actionMarchingCubes_triggered()
{
	vtkSmartPointer<vtkImageData> volume =vtkSmartPointer<vtkImageData>::New();
	//double isoValue=500;	
	QDir dir;
	QString fileName = QFileDialog::getExistingDirectory( this, QString(tr("open folder(only png image series can be read)")), dir.absolutePath() );
	if ( fileName.isEmpty() == true ) return;
	fileName+="/";
	// ֧�ִ�����·���Ķ�ȡ
	QByteArray ba = fileName.toLocal8Bit();
	const char *FilePrefix = ba.data();


	// �½��ļ���ȡ���󣬳�������vtkBMPReader��vtkDICOMImageReader��vtkJPEGReader��
	vtkSmartPointer<vtkPNGReader> pngReader =vtkSmartPointer<vtkPNGReader>::New();  
	// ��ͬ��reader��Ҫ���õĲ����ǲ�ͬ�� ��˱������ʺ�jpegreader
	//pngReader->SetFilePrefix("G:/GraduationDesignNeeded/ExperimentalData/Interpolation/OriginalImage/P01/"); // Ҫ�򿪵�·����ԭʼͼ��isoValueȡ500
	//pngReader->SetFilePrefix("G:/GraduationDesignNeeded/ExperimentalData/Interpolation/Image/P01/"); // Ҫ�򿪵�·�����ָ�󣨲�δ��ֵ����,isoValueȡ10    
    pngReader->SetFilePrefix(FilePrefix);
	pngReader->SetFilePattern("%s%03d.png");// ͼƬ�ļ�����ʽ���˴�Ϊ 001.png 015.png ...
	pngReader->SetDataByteOrderToLittleEndian();
	pngReader->SetDataSpacing(XDataSpace, YDataSpace, ZDataSpace);  // ����ͼƬ�����رȣ�
	pngReader->SetFileNameSliceSpacing(1); 
	//pngReader->SetDataExtent(0, 109, 0, 109, 1, 29);
	// ������Ϊ��P01�ļ���������0.png ~ 29.png����������Ϊ 0��29
	// ÿ��ͼƬ�ĳ���Ϊ110 * 110 �������Ϊ0��109
    pngReader->SetDataExtent(0, ImageLength-1, 0, ImageWidth-1, 0, NumberOfImages-1);
	pngReader->Update();

	double isoValue=QInputDialog::getDouble(NULL,tr(" "),tr("please input the isovalue:"));


	volume->DeepCopy(pngReader->GetOutput());
	vtkSmartPointer<vtkMarchingCubes> surface = vtkSmartPointer<vtkMarchingCubes>::New();
	surface->SetInputData(volume);
	surface->ComputeNormalsOn();
	surface->SetValue(0, isoValue);

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(surface->GetOutputPort());
	mapper->ScalarVisibilityOff();

	Ren2->RemoveActor(actor);

	actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper); 
	actor->GetProperty()->SetColor(1,0,0);
	Ren2->AddViewProp(actor);
	Ren2->ResetCamera();
}



void GUI::on_actionNewMarchingCubes_triggered()
{
	vtkSmartPointer<vtkImageData> OriginalVolume =vtkSmartPointer<vtkImageData>::New();
	vtkSmartPointer<vtkImageData> BinaryVolume =vtkSmartPointer<vtkImageData>::New();

	double isoValue=127;


	QDir Binarydir;
	QString BinaryfileName = QFileDialog::getExistingDirectory( this, 
		                                                   QString(tr("open folder containing binary image series(only png image series can be read)")), 
														   Binarydir.absolutePath() );
	if ( BinaryfileName.isEmpty() == true ) return;
	BinaryfileName+="/";
	// ֧�ִ�����·���Ķ�ȡ
	QByteArray Binaryba = BinaryfileName.toLocal8Bit();
	const char *BinaryFilePrefix = Binaryba.data();

	// �½��ļ���ȡ���󣬳�������vtkBMPReader��vtkDICOMImageReader��vtkJPEGReader��
	vtkSmartPointer<vtkPNGReader> BinaryPNGReader =vtkSmartPointer<vtkPNGReader>::New();  
	// ��ͬ��reader��Ҫ���õĲ����ǲ�ͬ�� ��˱������ʺ�jpegreader
	//BinaryPNGReader->SetFilePrefix("G:/GraduationDesignNeeded/ExperimentalData/Interpolation/BinaryImage16/P01/"); // Ҫ�򿪵�·��
    BinaryPNGReader->SetFilePrefix(BinaryFilePrefix); 
	BinaryPNGReader->SetFilePattern("%s%03d.png");// ͼƬ�ļ�����ʽ���˴�Ϊ 001.png 015.png ...
	BinaryPNGReader->SetDataByteOrderToLittleEndian();
	BinaryPNGReader->SetDataSpacing(XDataSpace, YDataSpace, ZDataSpace);  // ������ά���ݳ������ؼ�࣬
	BinaryPNGReader->SetFileNameSliceSpacing(1); 
	//BinaryPNGReader->SetDataExtent(0, 109, 0, 109, 1, 29);   //������һ�㣬��ʹ��������һ�����������������һ����ͨ����
	// ������Ϊ��P01�ļ���������0.png ~ 29.png����������Ϊ 0��29
	// ÿ��ͼƬ�ĳ���Ϊ110 * 110 �������Ϊ0��109
	BinaryPNGReader->SetDataExtent(0, ImageLength-1, 0, ImageWidth-1, 0, NumberOfImages-1); 
	BinaryPNGReader->Update();  
	BinaryVolume->DeepCopy(BinaryPNGReader->GetOutput());



	QDir Originaldir;
	QString OriginalfileName = QFileDialog::getExistingDirectory( this, 
		                                                   QString(tr("open folder containing original image series(only png image series can be read)")), 
														   Originaldir.absolutePath() );
	if ( OriginalfileName.isEmpty() == true ) return;
	OriginalfileName+="/";
	// ֧�ִ�����·���Ķ�ȡ
	QByteArray Originalba = OriginalfileName.toLocal8Bit();
	const char *OriginalFilePrefix = Originalba.data();

	// �½��ļ���ȡ���󣬳�������vtkBMPReader��vtkDICOMImageReader��vtkJPEGReader��
	vtkSmartPointer<vtkPNGReader> OriginalPNGReader =vtkSmartPointer<vtkPNGReader>::New();  
	// ��ͬ��reader��Ҫ���õĲ����ǲ�ͬ�� ��˱������ʺ�jpegreader
    //OriginalPNGReader->SetFilePrefix("G:/GraduationDesignNeeded/ExperimentalData/Interpolation/OriginalImage/P01/"); // Ҫ�򿪵�·��
    OriginalPNGReader->SetFilePrefix(OriginalFilePrefix);
	OriginalPNGReader->SetFilePattern("%s%03d.png");// ͼƬ�ļ�����ʽ���˴�Ϊ 001.png 015.png ...
	OriginalPNGReader->SetDataByteOrderToLittleEndian();
	OriginalPNGReader->SetDataSpacing(XDataSpace, YDataSpace, ZDataSpace);  // ������ά���ݳ������ؼ�࣬
	OriginalPNGReader->SetFileNameSliceSpacing(1); 
	//OriginalPNGReader->SetDataExtent(0, 109, 0, 109, 1, 30); //������һ�㣬��ʹ��������һ�����������������һ����ͨ����
	// ������Ϊ��P01�ļ���������0.png ~ 29.png����������Ϊ 0��29
	// ÿ��ͼƬ�ĳ���Ϊ110 * 110 �������Ϊ0��109
    OriginalPNGReader->SetDataExtent(0, ImageLength-1, 0, ImageWidth-1, 0, NumberOfImages-1); 
	OriginalPNGReader->Update(); 
	OriginalVolume->DeepCopy(OriginalPNGReader->GetOutput());


	

	vtkSmartPointer<vtkMarchingCubesSpace> surface = vtkSmartPointer<vtkMarchingCubesSpace>::New();

	surface->SetInputData(0,BinaryVolume);    //0�Ŷ˿���Ϊ��ֵ��ͼ������
	surface->SetInputData(1,OriginalVolume);  //1�Ŷ˿���Ϊԭʼͼ������˿�,���0�ź�1�Ŷ˿ڶ�����ͬһ�����ݣ����������㲻��

	surface->ComputeNormalsOn();
	//surface->ComputeNormalsOff();  //�����off����������Ĭ��Ϊ�����ֵ��ͼ��ķ��������㣬Ĭ��Ϊon
	surface->SetValue(0, isoValue);

	vtkSmartPointer<vtkStripper> stripper = 
		vtkSmartPointer<vtkStripper>::New();
	stripper->SetInputConnection(surface->GetOutputPort());  //�����ɵ�����Ƭ���ӳ����Ǵ� 
	stripper->Update();

	vtkSmartPointer<vtkSmoothPolyDataFilter> smoothFilter =
		vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
	smoothFilter->SetInputConnection(stripper->GetOutputPort());
	smoothFilter->SetNumberOfIterations(5);
	smoothFilter->Update();

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(smoothFilter->GetOutputPort());
	mapper->ScalarVisibilityOff();

	Ren2->RemoveActor(actor);

	actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);   
	actor->GetProperty()->SetColor(1,0,0);

	Ren2->AddViewProp(actor);
	Ren2->ResetCamera();
    
}