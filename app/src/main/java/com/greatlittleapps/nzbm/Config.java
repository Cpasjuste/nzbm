package com.greatlittleapps.nzbm;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.Charset;

import android.content.Context;

import com.greatlittleapps.utility.Utility;


public class Config {
    private File file;
    private String string = null;

    private int versionCode = 0;
    private String mainDir;
    private String downloadDir;
    private String interDir;
    private String webDir = "";
    private String configTemplate = "";
    private String unrarCmd = "";
    private String controlUsername = "nzbget";
    private String controlPassword = "nzbget";
    private String controlPort = "6789";
    private String outputMode = "loggable";
    private String logFile = mainDir + "/nzbget.log";
    public Paths paths;

    public Config(Context ctx) {
        paths = new Paths(ctx);
        file = new File(paths.config);
        if (!file.exists()) {
            Utility.log("could not find nzbget config file: " + paths.config);
            return;
        }
        load();
        parse();
    }

    public int getVersionCode() {

        return this.versionCode;
    }

    public String getControlUsername() {
        return this.controlUsername;
    }

    public void setControlUsername(String username) {
        this.string = this.string.replace("ControlUsername=" + this.controlUsername, "ControlPassword=" + username);
        this.controlUsername = username;
    }

    public String getControlPassword() {
        return this.controlPassword;
    }

    public void setControlPassword(String password) {
        this.string = this.string.replace("ControlPassword=" + this.controlPassword, "ControlPassword=" + password);
        this.controlPassword = password;
    }

    public String getControlPort() {
        return this.controlPort;
    }

    public void setControlPort(String port) {
        this.string = this.string.replace("ControlPort=" + this.controlPort, "ControlPort=" + port);
        this.controlPort = port;
    }

    public String getMainDir() {
        return this.mainDir;
    }

    public void setMainDir(String path) {
        this.string = this.string.replace("MainDir=" + this.mainDir, "MainDir=" + path);
        this.mainDir = path;
    }

    public String getDownloadDir() {
        return this.downloadDir;
    }

    public void setDownloadDir(String path) {
        this.string = this.string.replace("DestDir=" + this.downloadDir, "DestDir=" + path);
        this.downloadDir = path;
    }

    public String getInterDir() {
        return this.interDir;
    }

    public void setInterDir(String path) {
        this.string = this.string.replace("InterDir=" + this.interDir, "InterDir=" + path);
        this.interDir = path;
    }

    public String getWebDir() {
        return this.webDir;
    }

    public void setWebDir(String path) {
        this.string = this.string.replace("WebDir=" + this.webDir, "WebDir=" + path);
        this.webDir = path;
    }

    public String getConfigTemplate() {
        return this.configTemplate;
    }

    public void setConfigTemplate(String path) {
        this.string = this.string.replace("ConfigTemplate=" + this.configTemplate, "ConfigTemplate=" + path);
        this.configTemplate = path;
    }

    public String getunrarCmd() {
        return this.unrarCmd;
    }

    public void setunrarCmd(String path) {
        this.string = this.string.replace("UnrarCmd=" + this.unrarCmd, "UnrarCmd=" + path);
        this.unrarCmd = path;
    }

    public String getOutputmode() {
        return this.outputMode;
    }

    public void setOutputmode(String mode) {
        this.string = this.string.replace("OutputMode=" + this.outputMode, "OutputMode=" + mode);
        this.outputMode = mode;
    }

    public String getLogFile() {
        return this.unrarCmd;
    }

    public void setLogFile(String path) {
        this.string = this.string.replace("LogFile=" + this.logFile, "LogFile=" + path);
        this.logFile = path;
    }

    private void load() {
        try {
            FileInputStream inputStream = new FileInputStream(file);
            FileChannel fc = inputStream.getChannel();
            MappedByteBuffer buffer = fc.map(FileChannel.MapMode.READ_ONLY, 0, fc.size());
            string = Charset.defaultCharset().decode(buffer).toString();
            inputStream.close();
        } catch (IOException e) {
            e.printStackTrace();
            Utility.log(e.toString());
        }
    }

    private void parse() {
        int start, end = -1;

        if (string != null) {
            /*
			try
			{
				start = string.indexOf( "VersionCode=" ) + "VersionCode=".length();
				end = string.indexOf( '\n', start );
				versionCode = Integer.parseInt( string.substring( start, end ) );
			}
			catch( Exception e )
			{
				versionCode = 0;
				Utility.loge("VersionCode not found !");
			}
			*/

            start = string.indexOf("ControlPort=") + "ControlPort=".length();
            end = string.indexOf('\n', start);
            controlPort = string.substring(start, end);

            start = string.indexOf("ControlUsername=") + "ControlUsername=".length();
            end = string.indexOf('\n', start);
            controlUsername = string.substring(start, end);

            start = string.indexOf("ControlPassword=") + "ControlPassword=".length();
            end = string.indexOf('\n', start);
            controlPassword = string.substring(start, end);

            start = string.indexOf("MainDir=") + "MainDir=".length();
            end = string.indexOf('\n', start);
            mainDir = string.substring(start, end);

            start = string.indexOf("DestDir=") + "DestDir=".length();
            end = string.indexOf('\n', start);
            downloadDir = string.substring(start, end);

            start = string.indexOf("InterDir=") + "InterDir=".length();
            end = string.indexOf('\n', start);
            this.interDir = string.substring(start, end);

            start = string.indexOf("WebDir=") + "WebDir=".length();
            end = string.indexOf('\n', start);
            webDir = string.substring(start, end);

            start = string.indexOf("ConfigTemplate=") + "ConfigTemplate=".length();
            end = string.indexOf('\n', start);
            configTemplate = string.substring(start, end);

            start = string.indexOf("UnrarCmd=") + "UnrarCmd=".length();
            end = string.indexOf('\n', start);
            unrarCmd = string.substring(start, end);

            start = string.indexOf("OutputMode=") + "OutputMode=".length();
            end = string.indexOf('\n', start);
            outputMode = string.substring(start, end);

            start = string.indexOf("LogFile=") + "LogFile=".length();
            end = string.indexOf('\n', start);
            logFile = string.substring(start, end);

            //initialize mainDir to "sdcard/nzbget" if none is set
            if (mainDir.equals("~/downloads")
                    || !new File(paths.nzbget).exists()) {
                Utility.log("mainDir not set, setting to default");
                new File(paths.nzbget).mkdirs();
                this.setMainDir(paths.nzbget);
                this.setDownloadDir(paths.download);
                this.setInterDir(paths.download + "/nzbtemp");
                this.setWebDir(paths.webui);
                this.setConfigTemplate(paths.webui + "/nzbget.conf");
                this.setunrarCmd(paths.unrar + "/unrar");
                this.setOutputmode("loggable");
                this.setLogFile(paths.nzbget + "/nzbget.log");
                this.save();
            }

            if (!outputMode.equals("loggable")) {
                this.setOutputmode("loggable");
                this.save();
            }

            Utility.log("MainDir=" + mainDir);
        }
    }

    public void save() {
        try {
            FileWriter writer = new FileWriter(file);
            BufferedWriter out = new BufferedWriter(writer);
            out.write(this.string);
            out.close();
        } catch (java.io.IOException e) {
        }
    }

    public void close() {
        try {
            FileWriter writer = new FileWriter(file);
            BufferedWriter out = new BufferedWriter(writer);
            out.write(this.string);
            out.close();
        } catch (java.io.IOException e) {
        }
    }
}
