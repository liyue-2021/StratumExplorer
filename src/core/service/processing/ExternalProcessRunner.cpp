#include "ExternalProcessRunner.h"

#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// HDF5 support - 使用 hdf5 库读写配置文件
// 注意：需要链接 hdf5 库，CMakeLists.txt 中添加 find_package(HDF5) 并链接 hdf5
#ifdef USE_HDF5
#include <H5Cpp.h>
using namespace H5;
#endif

namespace processing
{

    ExternalProcessRunner::ExternalProcessRunner() = default;
    ExternalProcessRunner::~ExternalProcessRunner() = default;

    void ExternalProcessRunner::requestCancel()
    {
        m_cancel.store(true);
    }

    // ============================================================================
    // HDF5 配置写入（简化版，不依赖 hdf5 库时使用 JSON 模拟）
    // ============================================================================
    bool ExternalProcessRunner::writeInputConfig(const TaskRequest &req, const QString &configPath) const
    {
#ifdef USE_HDF5
        try
        {
            H5File file(configPath.toStdString(), H5F_CREATE_TRUNCATE);

            // /task_info 组
            Group taskInfoGroup(file.createGroup("/task_info"));
            {
                // task_id - int32
                int32_t taskId = req.taskId;
                DataSpace dataspace(DataSpace::SCALAR);
                DataSet dataset = taskInfoGroup.createDataSet("task_id", PredType::NATIVE_INT32, dataspace);
                dataset.write(&taskId, PredType::NATIVE_INT32);
            }
            {
                // func_id - int32
                int32_t funcId = req.funcId;
                DataSpace dataspace(DataSpace::SCALAR);
                DataSet dataset = taskInfoGroup.createDataSet("func_id", PredType::NATIVE_INT32, dataspace);
                dataset.write(&funcId, PredType::NATIVE_INT32);
            }
            {
                // func_name - string (variable length)
                StrType strType(PredType::C_S1, H5T_VARIABLE);
                DataSpace dataspace(DataSpace::SCALAR);
                DataSet dataset = taskInfoGroup.createDataSet("func_name", strType, dataspace);
                dataset.write(req.funcName.toStdString(), strType);
            }

            // /io 组
            Group ioGroup(file.createGroup("/io"));
            {
                // input_path - string[]
                hsize_t dims = req.inputPaths.size();
                DataSpace dataspace(1, &dims);
                StrType strType(PredType::C_S1, H5T_VARIABLE);
                DataSet dataset = ioGroup.createDataSet("input_path", strType, dataspace);
                std::vector<std::string> inputStrs;
                for (const auto &p : req.inputPaths)
                {
                    inputStrs.push_back(p.toStdString());
                }
                dataset.write(inputStrs, strType);
            }
            {
                // output_path - string
                StrType strType(PredType::C_S1, H5T_VARIABLE);
                DataSpace dataspace(DataSpace::SCALAR);
                DataSet dataset = ioGroup.createDataSet("output_path", strType, dataspace);
                dataset.write(req.outputPath.toStdString(), strType);
            }
            // 可选路径（仅当非空时才写入）
            if (!req.refDepthPath.isEmpty())
            {
                StrType strType(PredType::C_S1, H5T_VARIABLE);
                DataSpace dataspace(DataSpace::SCALAR);
                DataSet dataset = ioGroup.createDataSet("ref_depth_path", strType, dataspace);
                dataset.write(req.refDepthPath.toStdString(), strType);
            }
            if (!req.calibPath.isEmpty())
            {
                StrType strType(PredType::C_S1, H5T_VARIABLE);
                DataSpace dataspace(DataSpace::SCALAR);
                DataSet dataset = ioGroup.createDataSet("calib_path", strType, dataspace);
                dataset.write(req.calibPath.toStdString(), strType);
            }
            if (!req.tempPath.isEmpty())
            {
                StrType strType(PredType::C_S1, H5T_VARIABLE);
                DataSpace dataspace(DataSpace::SCALAR);
                DataSet dataset = ioGroup.createDataSet("temp_path", strType, dataspace);
                dataset.write(req.tempPath.toStdString(), strType);
            }
            if (!req.refPath.isEmpty())
            {
                StrType strType(PredType::C_S1, H5T_VARIABLE);
                DataSpace dataspace(DataSpace::SCALAR);
                DataSet dataset = ioGroup.createDataSet("ref_path", strType, dataspace);
                dataset.write(req.refPath.toStdString(), strType);
            }

            // /params 组（动态写入所有参数）
            Group paramsGroup(file.createGroup("/params"));
            for (auto it = req.params.constBegin(); it != req.params.constEnd(); ++it)
            {
                const QString key = it.key();
                const QVariant value = it.value();
                if (value.typeId() == QMetaType::Double)
                {
                    double d = value.toDouble();
                    DataSpace dataspace(DataSpace::SCALAR);
                    DataSet dataset = paramsGroup.createDataSet(key.toStdString(), PredType::NATIVE_DOUBLE, dataspace);
                    dataset.write(&d, PredType::NATIVE_DOUBLE);
                }
                else if (value.typeId() == QMetaType::Int || value.typeId() == QMetaType::LongLong)
                {
                    int i = value.toInt();
                    DataSpace dataspace(DataSpace::SCALAR);
                    DataSet dataset = paramsGroup.createDataSet(key.toStdString(), PredType::NATIVE_INT32, dataspace);
                    dataset.write(&i, PredType::NATIVE_INT32);
                }
                else if (value.typeId() == QMetaType::QString)
                {
                    StrType strType(PredType::C_S1, H5T_VARIABLE);
                    DataSpace dataspace(DataSpace::SCALAR);
                    DataSet dataset = paramsGroup.createDataSet(key.toStdString(), strType, dataspace);
                    dataset.write(value.toString().toStdString(), strType);
                }
            }

            return true;
        }
        catch (Exception &e)
        {
            qWarning() << "HDF5 write error:" << e.getCDetailMsg();
            return false;
        }
#else
        // 不使用 HDF5 库时的备选方案：生成 JSON 配置文件
        // 后端需要同时支持 JSON 格式的配置文件（通过环境变量或命令行参数切换）
        QJsonObject root;
        QJsonObject taskInfo;
        taskInfo["task_id"] = req.taskId;
        taskInfo["func_id"] = req.funcId;
        taskInfo["func_name"] = req.funcName;
        root["task_info"] = taskInfo;

        QJsonObject io;
        QJsonArray inputs;
        for (const auto &p : req.inputPaths)
            inputs.append(p);
        io["input_path"] = inputs;
        io["output_path"] = req.outputPath;
        if (!req.refDepthPath.isEmpty())
            io["ref_depth_path"] = req.refDepthPath;
        if (!req.calibPath.isEmpty())
            io["calib_path"] = req.calibPath;
        if (!req.tempPath.isEmpty())
            io["temp_path"] = req.tempPath;
        if (!req.refPath.isEmpty())
            io["ref_path"] = req.refPath;
        root["io"] = io;

        QJsonObject paramsJson = QJsonObject::fromVariantMap(req.params);
        root["params"] = paramsJson;

        QFile configFile(configPath);
        if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            return false;
        }
        configFile.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
        configFile.close();
        return true;
#endif
    }

    // ============================================================================
    // 读取任务结果文件 xxx_task.h5
    // ============================================================================
    ExternalProcessRunner::TaskResult ExternalProcessRunner::readTaskResult(const QString &taskPath) const
    {
        TaskResult result;

#ifdef USE_HDF5
        try
        {
            H5File file(taskPath.toStdString(), H5F_ACC_RDONLY);

            // /status/code
            {
                DataSet dataset = file.openDataSet("/status/code");
                int code;
                dataset.read(&code, PredType::NATIVE_INT32);
                result.status = (code == 0) ? NodeStatus::Succeeded : NodeStatus::Failed;
            }

            // /status/message
            if (result.status == NodeStatus::Failed)
            {
                DataSet dataset = file.openDataSet("/status/message");
                std::string msg;
                dataset.read(msg, PredType::C_S1);
                result.message = QString::fromStdString(msg);
            }

            // /io/data_file_path
            try
            {
                DataSet dataset = file.openDataSet("/io/data_file_path");
                std::string path;
                dataset.read(path, PredType::C_S1);
                result.dataFilePath = QString::fromStdString(path);
            }
            catch (Exception &)
            {
                // 可选字段，忽略
            }

            // /io/output_path (string[])
            try
            {
                DataSet dataset = file.openDataSet("/io/output_path");
                // 可能返回的是字符串或字符串数组
                DataSpace space = dataset.getSpace();
                hsize_t dims[1];
                space.getSimpleExtentDims(dims);
                if (dims[0] == 1)
                {
                    std::string path;
                    dataset.read(path, PredType::C_S1);
                    result.outputPaths.append(QString::fromStdString(path));
                }
            }
            catch (Exception &)
            {
                // 可选字段，忽略
            }

            // /time/duration
            try
            {
                DataSet dataset = file.openDataSet("/time/duration");
                float duration;
                dataset.read(&duration, PredType::NATIVE_FLOAT);
                result.duration = static_cast<int>(duration * 1000); // 转换为毫秒
            }
            catch (Exception &)
            {
                // 可选字段，忽略
            }
        }
        catch (Exception &e)
        {
            result.status = NodeStatus::Failed;
            result.message = QStringLiteral("读取任务结果文件失败: %1").arg(e.getCDetailMsg());
        }
#else
        // 不使用 HDF5 库时的备选方案：读取 JSON 格式结果
        QFile taskFile(taskPath);
        if (!taskFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            result.status = NodeStatus::Failed;
            result.message = QStringLiteral("无法打开任务结果文件: %1").arg(taskPath);
            return result;
        }

        QByteArray data = taskFile.readAll();
        taskFile.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject())
        {
            result.status = NodeStatus::Failed;
            result.message = QStringLiteral("任务结果文件格式错误");
            return result;
        }

        QJsonObject root = doc.object();

        // /status/code
        int code = root["status"].toObject()["code"].toInt();
        result.status = (code == 0) ? NodeStatus::Succeeded : NodeStatus::Failed;

        // /status/message
        result.message = root["status"].toObject()["message"].toString();

        // /io/data_file_path
        result.dataFilePath = root["io"].toObject()["data_file_path"].toString();

        // /io/output_path (可能是字符串或数组)
        QJsonValue outputVal = root["io"].toObject()["output_path"];
        if (outputVal.isArray())
        {
            for (const auto &v : outputVal.toArray())
            {
                result.outputPaths.append(v.toString());
            }
        }
        else if (outputVal.isString())
        {
            result.outputPaths.append(outputVal.toString());
        }

        // /time/duration (秒 -> 毫秒)
        result.duration = static_cast<int>(root["time"].toObject()["duration"].toDouble() * 1000);
#endif

        return result;
    }

    // ============================================================================
    // 主执行函数：生成配置 → 启动 EXE → 读取结果
    // ============================================================================
    ExternalProcessRunner::TaskResult
    ExternalProcessRunner::runBlocking(const TaskRequest &req, ProgressFn onProgress)
    {
        TaskResult r;
        m_cancel.store(false);

        if (req.exePath.isEmpty() || !QFileInfo::exists(req.exePath))
        {
            r.status = NodeStatus::Failed;
            r.message = QStringLiteral("算法可执行文件不存在: %1").arg(req.exePath);
            return r;
        }

        // 生成临时配置文件路径
        QString configDir = req.workingDir.isEmpty()
                                ? QFileInfo(req.exePath).absolutePath()
                                : req.workingDir;
        if (!QDir(configDir).exists())
        {
            configDir = QDir::tempPath();
        }

#ifdef USE_HDF5
        const QString configExt = QStringLiteral("h5");
#else
        // 前端默认写 JSON 任务描述；HDF5 二进制编解码由后端 EXE 负责
        const QString configExt = QStringLiteral("json");
#endif
        const QString configFileName =
            QStringLiteral("input_config_%1.%2").arg(req.taskId).arg(configExt);
        const QString configPath = configDir + QLatin1Char('/') + configFileName;

        // 生成输入配置文件
        if (!writeInputConfig(req, configPath))
        {
            r.status = NodeStatus::Failed;
            r.message = QStringLiteral("生成输入配置文件失败: %1").arg(configPath);
            return r;
        }

        // 通知进度：配置已生成
        if (onProgress)
        {
            onProgress(5, QStringLiteral("配置已生成"));
        }

        // 启动进程，传入配置文件路径作为唯一参数
        QProcess proc;
        m_proc = &proc;
        proc.setProgram(req.exePath);
        proc.setArguments(QStringList() << configPath); // 唯一参数：任务配置文件路径
        proc.setWorkingDirectory(configDir);

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifdef USE_HDF5
        env.insert(QStringLiteral("OILPRO_PROTOCOL"), QStringLiteral("HDF5"));
#else
        env.insert(QStringLiteral("OILPRO_PROTOCOL"), QStringLiteral("JSON"));
#endif
        proc.setProcessEnvironment(env);

        // 用本地事件循环把异步 QProcess 转成同步
        QEventLoop loop;
        bool finished = false;
        int exitCode = -1;
        QProcess::ExitStatus exitStatus = QProcess::NormalExit;
        qint64 startTime = QDateTime::currentMSecsSinceEpoch();

        // stderr 转发到日志
        QObject::connect(&proc, &QProcess::readyReadStandardError, [&]
                         {
        const QByteArray err = proc.readAllStandardError();
        if (!err.isEmpty() && onProgress) {
            onProgress(-1, QStringLiteral("[算法] %1").arg(QString::fromUtf8(err).trimmed()));
        } });

        // 进程结束
        QObject::connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         [&](int code, QProcess::ExitStatus st)
                         {
                             exitCode = code;
                             exitStatus = st;
                             finished = true;
                             loop.quit();
                         });

        // 周期性检查取消标志：terminate → 5s 后 kill
        QTimer cancelTick;
        cancelTick.setInterval(200);
        bool terminateSent = false;
        qint64 termAtMs = 0;
        QObject::connect(&cancelTick, &QTimer::timeout, [&]
                         {
        if (!m_cancel.load()) return;
        if (!terminateSent && proc.state() == QProcess::Running) {
            proc.terminate();
            terminateSent = true;
            termAtMs = QDateTime::currentMSecsSinceEpoch();
        } else if (terminateSent && proc.state() == QProcess::Running) {
            if (QDateTime::currentMSecsSinceEpoch() - termAtMs > 5000) {
                proc.kill();
            }
        } });
        cancelTick.start();

        proc.start();
        if (!proc.waitForStarted(5000))
        {
            m_proc = nullptr;
            r.status = NodeStatus::Failed;
            r.message = QStringLiteral("无法启动: %1").arg(proc.errorString());
            cancelTick.stop();
            return r;
        }

        // 通知进度：进程已启动
        if (onProgress)
        {
            onProgress(10, QStringLiteral("算法进程已启动"));
        }

        // 等待进程结束
        if (!finished)
            loop.exec();

        cancelTick.stop();
        m_proc = nullptr;

        // 计算耗时
        qint64 endTime = QDateTime::currentMSecsSinceEpoch();
        r.duration = static_cast<int>(endTime - startTime);

        // 进程正常退出：读取任务结果文件
        if (exitStatus == QProcess::NormalExit)
        {
            // 生成期望的任务结果文件路径（在 outputPath 下）
            const QString taskFileName =
                QStringLiteral("task_%1.%2").arg(req.taskId).arg(configExt);
            QString taskPath = req.outputPath.isEmpty()
                                   ? (configDir + "/" + taskFileName)
                                   : (req.outputPath + "/" + taskFileName);

            // 检查任务结果文件是否存在
            if (QFileInfo::exists(taskPath))
            {
                r = readTaskResult(taskPath);
                if (onProgress)
                {
                    onProgress(100, r.status == NodeStatus::Succeeded
                                        ? QStringLiteral("完成，耗时 %1 ms").arg(r.duration)
                                        : QStringLiteral("失败: %1").arg(r.message));
                }
            }
            else
            {
                // 备选：尝试从进程输出获取结果文件路径
                QString stdoutData = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
                if (!stdoutData.isEmpty() && QFileInfo::exists(stdoutData))
                {
                    r = readTaskResult(stdoutData);
                }
                else if (exitCode == 0)
                {
                    // 进程成功但没有结果文件，认为成功
                    r.status = NodeStatus::Succeeded;
                    r.message = QStringLiteral("算法执行成功（无结果文件）");
                }
                else
                {
                    r.status = NodeStatus::Failed;
                    r.message = QStringLiteral("未找到任务结果文件: %1").arg(taskPath);
                }
            }
        }
        else
        {
            // 进程异常退出
            r.status = NodeStatus::Failed;
            r.message = QStringLiteral("算法进程异常退出 (exit=%1)").arg(exitCode);
        }

        // 取消状态处理
        if (m_cancel.load())
        {
            r.status = NodeStatus::Stopped;
            r.message = QStringLiteral("用户停止");
        }

        // 清理临时配置文件
        QFile::remove(configPath);

        return r;
    }

} // namespace processing