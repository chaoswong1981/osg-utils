
/*
	用于对3DMax导出的带有动画的模型进行开关控制
	
	要求：
	1. 导出的模型只包含一个完整帧动画
	2. 该动画仅包含打开或关闭的动画，另一半动画由程序逆向播放动画来完成

	应用方法：
	在场景节点 root 上，执行：
	AnimationSetting v;
	root->accpet(v);
	即可

	完成后，左键双击执行“开”动画；右键双击执行“关”动画
*/


#include <osgGA/GUIEventHandler>
#include <osgUtil/LineSegmentIntersector>
#include <osg/PositionAttitudeTransform>
#include <osg/AnimationPath>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>

class OpenCloseEventCallback : public osgGA::GUIEventHandler
{
public:
	OpenCloseEventCallback(osg::Node* node) {
		_open = false;
		_node = node;
	}
	~OpenCloseEventCallback() {}

	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) {
		if (ea.getEventType() == osgGA::GUIEventAdapter::DOUBLECLICK)
		{
            if (ea.getButtonMask() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
                _open = true;
            else
                _open = false;

			osgViewer::Viewer* view = dynamic_cast<osgViewer::Viewer*>(&aa);
			osgUtil::LineSegmentIntersector::Intersections inters;
			if (view->computeIntersections(ea.getX(), ea.getY(), inters))
			{
				osgUtil::LineSegmentIntersector::Intersections::iterator iter =
					inters.begin();

				osg::Node* node = *(iter->nodePath.rbegin()+2); // 跳过 -GEODE 和 -OFFSET
				if (node == _node)
				{
					osg::AnimationPathCallback* apc = dynamic_cast<osg::AnimationPathCallback*>(node->getUpdateCallback());
					apc->setPause(false);
					apc->reset();

					if (_open)
						apc->setTimeOffset(0);
					else
						apc->setTimeOffset(-apc->getAnimationPath()->getPeriod());
				}
			}
		}
		return false;
	}

	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
	{
		if (nv && nv->getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR)
		{
			osgGA::EventVisitor* ev = dynamic_cast<osgGA::EventVisitor*>(nv);

			osg::AnimationPathCallback* apc = dynamic_cast<osg::AnimationPathCallback*>(node->getUpdateCallback());
			if (apc && !apc->getPause())
			{
                //动画的播放方式改为了SWING，所以_open==true时，播放前半段，否则播放后半段
				if (_open)
				{
					if (apc->getAnimationTime() >= apc->getAnimationPath()->getPeriod())
						apc->setPause(true);
				}
				else
				{
					if (apc->getAnimationTime() >= apc->getAnimationPath()->getPeriod()*2)
						apc->setPause(true);
				}
			}
		}
		else
		{
			osgGA::EventVisitor* ev = dynamic_cast<osgGA::EventVisitor*>(nv);
            if (ev && ev->getActionAdapter() && !ev->getEvents().empty())
            {
                for(osgGA::EventQueue::Events::iterator itr = ev->getEvents().begin();
                    itr != ev->getEvents().end();
                    ++itr)
                {
                    handleWithCheckAgainstIgnoreHandledEventsMask(*(*itr), *(ev->getActionAdapter()), node, nv);
                }
            }
		}
		traverse(node, nv);
	}

protected:
	bool _open;
	osg::Node* _node;
};

class AnimationSetting : public osg::NodeVisitor
{
public:
	AnimationSetting() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
	virtual void apply(osg::Node& node)
	{
		osg::AnimationPathCallback* apc = dynamic_cast<osg::AnimationPathCallback*>(node.getUpdateCallback());
		if (apc)
		{
			apc->getAnimationPath()->setLoopMode(osg::AnimationPath::SWING);
			apc->setPause(true);

			OpenCloseEventCallback* callback = new OpenCloseEventCallback(&node);
			node.setEventCallback(callback);
			node.addUpdateCallback(callback);
		}
		traverse(node);
	}
};

osg::Node* createModel1()
{
	osg::Node* node = osgDB::readNodeFile("fm.ive");
	AnimationSetting v;
	node->accept(v);

	return node;
}

int main(int argc, char **argv)
{
	osg::ArgumentParser arguments(&argc, argv);
	osgViewer::Viewer viewer;

	osg::Node* root = createModel1();
	osg::Node* root1 = createModel1();
	osg::PositionAttitudeTransform* pat = new osg::PositionAttitudeTransform;
	pat->setPosition(osg::Vec3(1, 0, 0));
	pat->addChild(root1);

	osg::Group* grp = new osg::Group;
	grp->addChild(root);
	grp->addChild(pat);

	//UpdateCallback* uc = new UpdateCallback;
	//root->setUpdateCallback(uc);

	viewer.setSceneData(grp);

	viewer.setCameraManipulator(new osgGA::TrackballManipulator);
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);
	//viewer.addEventHandler(new EventHandler(uc));

	return viewer.run();
}
