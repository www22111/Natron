//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com


#include "Writer/Writer.h"
#include "Gui/knob_callback.h"
#include "Core/row.h"
#include "Writer/Write.h"
#include "Gui/knob.h"
#include "Core/settings.h"
#include "Superviser/controler.h"
#include "Core/model.h"
#include <QtConcurrent/QtConcurrent>
#include "Core/settings.h"

using namespace std;

Writer::Writer(Node* node):OutputNode(node),_writeHandle(0),_currentFrame(0),_writeOptions(0),
_buffer(Settings::getPowiterCurrentSettings()->_writersSettings._maximumBufferSize),_premult(false){
    
    /*debug*/
    _requestedChannels = Mask_RGB;
}

Writer::Writer(Writer& ref):OutputNode(ref),_writeHandle(0),_currentFrame(0),_writeOptions(ref._writeOptions),
_buffer(Settings::getPowiterCurrentSettings()->_writersSettings._maximumBufferSize),_premult(ref._premult){
    
}

Writer::~Writer(){
    if(_writeOptions){
        _writeOptions->cleanUpKnobs();
        delete _writeOptions;
    }
}

std::string Writer::className(){
    return "Writer";
}

std::string Writer::description(){
    return "OutputNode";
}

void Writer::_validate(bool forReal){
    if(_parents.size()==1){
		copy_info(_parents[0],forReal);
	}
    
    /*Defaults writing range to readers range, but 
     the user may change it through GUI.*/
    _frameRange.first = _info->firstFrame();
    _frameRange.second = _info->lastFrame();
    
    setOutputChannels(Mask_All);
    
    if(forReal){
        if(_filename.size() > 0){
                        
            Write* write = 0;
            PluginID* encoder = Settings::getPowiterCurrentSettings()->_writersSettings.encoderForFiletype(_fileType);
            if(!encoder){
                cout << "Writer: couldn't find an appropriate encoder for filetype: " << _fileType << endl;
            }else{
                WriteBuilder builder = (WriteBuilder)(encoder->first);
                write = builder(this);
                write->premultiplyByAlpha(_premult);
                /*check if the filename already contains the extension, otherwise appending it*/
                QString extension;
                QString filename(_filename.c_str());
                int i = filename.size()-1;
                while(i >= 0 && filename.at(i) != QChar('.')){extension.prepend(filename.at(i));i--;}
                
                PluginID* isValid = Settings::getPowiterCurrentSettings()->_writersSettings.encoderForFiletype(extension.toStdString());
                QString frameNumber;
                char tmp[50];
                sprintf(tmp, "%i",_currentFrame);
                frameNumber.append(tmp);
                if(!isValid || extension.toStdString()!=_fileType){ //extension not contained in filename, append it
                    filename.append(frameNumber);
                    filename.append(".");
                    filename.append(_fileType.c_str());
                }else{
                    i--; /*i was at the '.' character, we put it one letter before so we can insert the frame number*/
                    filename.insert(i, frameNumber);
                }
                write->setOptionalKnobsPtr(_writeOptions);
                write->setupFile(filename.toStdString());
                write->initializeColorSpace();
                _writeHandle = write;
            }
        }
    }
}

void Writer::engine(int y,int offset,int range,ChannelMask channels,Row* out){
    _writeHandle->engine(y, offset, range, channels, out);
}

void Writer::createKnobDynamically(){
    Node::createKnobDynamically();
}
void Writer::initKnobs(Knob_Callback *cb){
    std::string fileDesc("File");
    OutputFile_Knob* file = static_cast<OutputFile_Knob*>(KnobFactory::createKnob("OutputFile", cb, fileDesc, Knob::NONE));
    assert(file);
	file->setPointer(&_filename);
    std::string renderDesc("Render");
    Button_Knob* renderButton = static_cast<Button_Knob*>(KnobFactory::createKnob("Button", cb, renderDesc, Knob::NONE));
    assert(renderButton);
    renderButton->connectButtonToSlot(dynamic_cast<QObject*>(this),SLOT(startRendering()));
    
    std::string premultString("Premultiply by alpha");
    Bool_Knob* premult = static_cast<Bool_Knob*>(KnobFactory::createKnob("Bool", cb, premultString, Knob::NONE));
    premult->setPointer(&_premult);

    std::string filetypeStr("File type");
    ComboBox_Knob* filetypeCombo = static_cast<ComboBox_Knob*>(KnobFactory::createKnob("ComboBox", cb, filetypeStr, Knob::NONE));
    QObject::connect(filetypeCombo, SIGNAL(entryChanged(std::string&)), this, SLOT(fileTypeChanged(std::string&)));
    vector<string> entries;
    const std::map<std::string,PluginID*>& _encoders = Settings::getPowiterCurrentSettings()->_writersSettings.getFileTypesMap();
    std::map<std::string,PluginID*>::const_iterator it = _encoders.begin();
    for(;it!=_encoders.end();it++){
        entries.push_back(it->first);
    }
    filetypeCombo->populate(entries);
    filetypeCombo->setPointer(&_fileType);
    Node::initKnobs(cb);
}


void Writer::write(Write* write,QFutureWatcher<void>* watcher){
//    while (_buffer.size() >= _buffer.getMaximumBufferSize()) {
//        //active waiting for this thread
//    }
    _buffer.appendTask(write, watcher);
    if(!write) return;
    write->writeAndDelete();
    _buffer.removeTask(write);
}

void Writer::startWriting(){
        QFutureWatcher<void>* watcher = new QFutureWatcher<void>;
        QObject::connect(watcher, SIGNAL(finished()), this, SLOT(notifyWriterForCompletion()));
        QFuture<void> future = QtConcurrent::run(this,&Writer::write,_writeHandle,watcher);
        _writeHandle = 0;
        watcher->setFuture(future);
}

void Writer::notifyWriterForCompletion(){
    _buffer.emptyTrash();
}

void Writer::Buffer::appendTask(Write* task,QFutureWatcher<void>* future){
    _tasks.push_back(make_pair(task, future));
}

void Writer::Buffer::removeTask(Write* task){
    for (U32 i = 0 ; i < _tasks.size(); i++) {
        std::pair<Write*,QFutureWatcher<void>* >& t = _tasks[i];
        if(t.first == task){
            _trash.push_back(t.second);
            _tasks.erase(_tasks.begin()+i);
            break;
        }
    }
}
void Writer::Buffer::emptyTrash(){
    for (U32 i = 0; i < _trash.size(); i++) {
        delete _trash[i];
    }
    _trash.clear();
}

Writer::Buffer::~Buffer(){
    for (U32 i = 0 ; i < _tasks.size(); i++) {
        std::pair<Write*,QFutureWatcher<void>* >& t = _tasks[i];
        delete t.second;
    }
}

bool Writer::validInfosForRendering(){
    /*check if filetype is valid*/
    PluginID* isValid = Settings::getPowiterCurrentSettings()->_writersSettings.encoderForFiletype(_fileType);
    if(!isValid) return false;
    
    /*checking if channels are supported*/
    WriteBuilder builder = (WriteBuilder)isValid->first;
    Write* write = builder(NULL);
    try{
        write->supportsChannelsForWriting(_requestedChannels);
    }catch(const char* str){
        cout << "ERROR: " << str << endl;
    }
    delete write;
    
    /*check if frame range makes sense*/
    if(_frameRange.first > _frameRange.second) return false;
    
    /*check if write specific knobs have valid values*/
    if (_writeOptions) {
        if (!_writeOptions->allValid()) {
            return false;
        }
    }
    
    return true;
}

void Writer::startRendering(){
    if(validInfosForRendering()){
        ctrlPTR->getModel()->setVideoEngineRequirements(this);
        ctrlPTR->getModel()->startVideoEngine();
    }
}

void Writer::fileTypeChanged(std::string& fileType){
    if(_writeOptions){
        _writeOptions->cleanUpKnobs();
        delete _writeOptions;
        _writeOptions = 0;
    }
    PluginID* isValid = Settings::getPowiterCurrentSettings()->_writersSettings.encoderForFiletype(_fileType);
    if(!isValid) return;
    
    /*checking if channels are supported*/
    WriteBuilder builder = (WriteBuilder)isValid->first;
    Write* write = builder(this);
    _writeOptions = write->initSpecificKnobs();
    if(_writeOptions)
        _writeOptions->initKnobs(getKnobCallBack(),fileType);
    delete write;
}