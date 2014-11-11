using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Security.Cryptography;
using System.IO;
using Microsoft.Win32;
using System.Reflection;

namespace RamdiskBuilder {
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window {

        static string baseDir = Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);
        
        static string tarFile = Path.Combine(baseDir, @"ssh.tar");
        
        static string dllFile = Path.Combine(baseDir, @"dmglib.dll");
        

        public MainWindow() {
            InitializeComponent();
            SanityChecks();
        }

        private void Log(string format, params object[] args) {
            logText.Text += string.Format("\r\n" +  format, args);
        }

        void SanityChecks() {
            bool pass = true;
            if (!File.Exists(tarFile)) {
                pass = false;
                Log("ERROR! Sanity check FAILED: {0} NOT found", tarFile);
            }
            if (!File.Exists(dllFile)) {
                pass = false;
                Log("ERROR! Sanity check FAILED: {0} NOT found", dllFile);
            }
            
            if (pass) {
                Log("Sanity checks PASSED");
            }
        }

        void ProcessFile(string droppedFile) {

            SHA1 sha = SHA1.Create();
            using (FileStream dmgStream = new FileStream(droppedFile, FileMode.Open)) {

                byte[] hash = sha.ComputeHash(dmgStream);
                string strHash = hash.Aggregate<byte, StringBuilder, string>(new StringBuilder(), (acc, b) => acc.AppendFormat("{0:X2}", b), sb => sb.ToString());
                Log("SHA1 Hash: {0}", strHash);
            }
            try {
                string ivStr = ivString.Text;// "fd19726dc6b555b6bb4dbbcd91d1e7c0";
                string keyStr = keyString.Text; // "fb2792b935fb9cd183341cb24539376556f8b7b8f887eb90fcebaa0daf2d6d9c";

                string kbagHexStr = xpwn.xpwntool_get_kbag(droppedFile);

                if (!string.IsNullOrEmpty(kbagHexStr)) {
                    Log("KBAG: {0}", kbagHexStr);
                }

                string decFile = droppedFile + ".dec";
                if (0 != xpwn.xpwntool_enc_dec(droppedFile, decFile, null, ivStr, keyStr)) {
                    Log("Decrypt failed!");
                    throw new Exception("Decrypt failed!");
                }
                Log("Decrypt OK (does not prove the key is correct)");

                IntPtr ctx = IntPtr.Zero;
                try {
                    ctx = Hfs.hfslib_open(decFile);
                } catch (Exception) {
                    Log("Hfs.hfslib_open() failed; recheck the iv/key!");
                    throw;
                }
                Log("HFS opened OK (key is good)");


                UInt64 currentSize = Hfs.hfslib_getsize(ctx);
                FileInfo fi = new FileInfo(tarFile);
                Log("Volume size: {0}, tar size: {1}", currentSize, fi.Length);
                UInt64 deltaSize = (UInt64)(1.05 * (double)fi.Length);
                if (!Hfs.hfslib_extend(ctx, currentSize + deltaSize)) {
                    Log("Extend failed!");
                    throw new Exception("Extend failed!");
                }
                Log("HFS Extend OK, extended {0} bytes", deltaSize);


                if (!Hfs.hfslib_untar(ctx, tarFile)) {
                    Log("Untar failed!");
                    throw new Exception("Untar failed!");
                }
                Log("Untar OK");

                Hfs.hfslib_close(ctx);
                string sshFile = droppedFile + ".ssh";
                if (0 != xpwn.xpwntool_enc_dec(decFile, sshFile, droppedFile, ivStr, keyStr)) {
                    Log("Encrypt failed!");
                } else {
                    Log("Encrypt OK");
                    Log("ALL OK; boot with '{0}' ramdisk and a jailbroken kernelcache!", Path.GetFileName(sshFile));                    
                }
            
            }
            catch (Exception ex) {
                Console.WriteLine("Exception: {0}; {1}", ex.Message, ex.StackTrace);
                Log("Exception: {0}; {1}", ex.Message, ex.StackTrace);
            }
        }

        private void button1_Click(object sender, RoutedEventArgs e) {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.CheckFileExists = true;
            ofd.Filter = "Disk image files|*.dmg|All Files|*.*";
            bool? dialogResult = ofd.ShowDialog();
            if (dialogResult == true) {
                selectedFile.Text = ofd.FileName;
                ProcessFile(ofd.FileName);
            }
        }

    }
}
