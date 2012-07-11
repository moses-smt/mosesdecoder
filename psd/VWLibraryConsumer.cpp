VWLibraryConsumer::VWLibraryConsumer()
{
    m_vwInstance = new VW::initialize("--hash all -q st --noconstant -i " + modelFile);
    m_ex = new ezexample(m_vwInstance,false);
}

//Destructor

VWLibraryConsumer::SetNamespace(const string &ns, bool shared)
{
    if(!m_shared)
    {
        --(*m_ex);
    }
    (*m_ex)(vw_namespace(ns));
    m_shared = shared;
}

VWLibraryConsumer::FinishExample()
{
    delete m_ex;
    m_ex = new ezexample(m_vwInstance,false);
}

VWLibraryConsumer::Finish()
{
    finish(m_VWInstance);
}

VWLibraryConsumer::AddFeature(const string &name)
{
    m_ex(name);
}


VWLibraryConsumer::AddFeature(const string &name, real value)
{
    (*m_ex)(name,value);
}

VWLibraryPredictConsumer::Predict(const string &label)
{
    return m_ex->predict();
}

VWLibraryTrainConsumer::Train(const string &label, float loss)
{
  //???
}

VWFileTrainConsumer::VWFileBasedConsumer(const string &outputFile)
{
    ostream os;
    os.open();
    outputFile >> ss;
}
