using System;
using System.Linq;
using System.Windows;
using System.Windows.Forms;             // для NumericUpDown
using System.Windows.Forms.Integration; // для WindowsFormsHost
using System.Runtime.InteropServices;
using System.IO;
using System.Net;                       // для IPAddress
using System.Drawing;                   // для настройки шрифта 
using System.ComponentModel;            // для PropertyChangedEventArgs

namespace IPcalcWPFv2
{
	
	public partial class MainWindow : Window
	{
		[DllImport("kernel32.dll", SetLastError = true)]
		static extern bool AllocConsole();

		[DllImport("kernel32.dll", SetLastError = true)]
		static extern bool FreeConsole();

		int? _lastPrintedPrefix = null;                                     // Храним последний напечатанный префикс
		string _lastPrintedIp = null;                                       // Храним последний напечатанный IP
		public MainWindow()
		{
			AllocConsole();             
			InitializeComponent();
			Console.WriteLine("Консоль подключена");


			Loaded += (s, e) => ipAddressField.FirstByteTextBox.Focus();    // Ставим фокус

			ipAddressField.IpAddress = "127.0.0.1";							// Устанавливаем IP по умолчанию
			IpField_PropertyChanged(ipAddressField, new PropertyChangedEventArgs("IpFirstByte")); // Вызываем обработку
			
			ipAddressField.PropertyChanged += IpField_PropertyChanged;      // событие изменения IP-адреса
			numericUpDownPrefix.ValueChanged += Prefix_ValueChanged;        // событие изменения префикса (NumericUpDown)
		}
		void IpField_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
		{
			if (e.PropertyName == "IpFirstByte")                            // первый байт
			{
				if (byte.TryParse(ipAddressField.IpFirstByte, out byte first)) // преобразовать первый байт в число
				{
					int prefix;                                                // Определяем префикс по классу IP-адреса
					if (first < 128) prefix = 8;
					else if (first < 192) prefix = 16;
					else if (first < 224) prefix = 24;
					else prefix = 32;

					if (_lastPrintedPrefix != prefix)						
					{
						Console.WriteLine($"Префикс рассчитан: /{prefix}");
						_lastPrintedPrefix = prefix;
					}

					numericUpDownPrefix.Value = prefix;                         // Устанавливаем новый префикс в NumericUpDown
					UpdateNetworkInfo(ipAddressField.IpAddress, prefix);        // Обновляем расчёт сети и broadcast
					
					// Построение маски
					uint maskBits = prefix == 0 ? 0u : 0xFFFFFFFFu << (32 - prefix);
					string maskText = string.Format("{0}.{1}.{2}.{3}",
						(maskBits >> 24) & 0xFF,
						(maskBits >> 16) & 0xFF,
						(maskBits >> 8) & 0xFF,
						maskBits & 0xFF);

					// Устанавливаем в маску
					maskField.IpAddress = maskText;

					//Console.WriteLine($"Префикс рассчитан: /{prefix}");
					if (_lastPrintedIp != ipAddressField.IpAddress)
					{
						Console.WriteLine($"IP-адрес: {ipAddressField.IpAddress}");
						_lastPrintedIp = ipAddressField.IpAddress;
					}
				}
			}
		}

		private void Prefix_ValueChanged(object sender, EventArgs e)
		{
			int prefix = (int)numericUpDownPrefix.Value;                            // Получаем новое значение префикса

			// Построение маски
			uint maskBits = prefix == 0 ? 0u : 0xFFFFFFFFu << (32 - prefix);        // Вычисляем битовую маску из префикса
			
			string maskText = string.Format("{0}.{1}.{2}.{3}",                      // Строим строку маски
				(maskBits >> 24) & 0xFF,
				(maskBits >> 16) & 0xFF,
				(maskBits >> 8) & 0xFF,
				maskBits & 0xFF);

			maskField.IpAddress = maskText;                                         // Устанавливаем маску

			// Получаем IP
			if (IPAddress.TryParse(ipAddressField.IpAddress, out IPAddress ip))     // Получаем текущий IP, переводим его в uint
			{
				byte[] ipBytes = ip.GetAddressBytes();
				uint ipUint = BitConverter.ToUInt32(ipBytes.Reverse().ToArray(), 0);

				//uint netUint = ipUint & maskBits;                                   // Находим сетевой адрес (минимальный)

				// Переводим сетевой адрес обратно в строку IP
				//byte[] netBytes = BitConverter.GetBytes(netUint).Reverse().ToArray();
				//string netIp = string.Join(".", netBytes.Select(b => b.ToString()));

				// Обновляем IP на сетевой адрес (минимальный)
				//ipAddressField.IpAddress = netIp;

				
				UpdateNetworkInfo(ipAddressField.IpAddress, prefix);                // Перерасчёт остальной информации
			}
		}

		void UpdateNetworkInfo(string ipText, int prefix)
		{
			if (!IPAddress.TryParse(ipText, out IPAddress ip))
				return;

			// Расчет маски
			uint maskBits = prefix == 0 ? 0u : 0xFFFFFFFFu << (32 - prefix);            // Строим маску

			// Сетевой адрес  // Переводим IP в uint
			byte[] ipBytes = ip.GetAddressBytes();
			byte[] reversedIpBytes = ipBytes.Reverse().ToArray();
			uint ipUint = BitConverter.ToUInt32(reversedIpBytes, 0);

			// Вычисляем сетевой адрес
			uint netUint = ipUint & maskBits;
			byte[] netBytes = BitConverter.GetBytes(netUint).Reverse().ToArray();
			string networkAddress = string.Join(".", netBytes);

			// Broadcast адрес
			uint broadcastUint = ipUint | ~maskBits;
			byte[] broadcastBytes = BitConverter.GetBytes(broadcastUint).Reverse().ToArray();
			string broadcastAddress = string.Join(".", broadcastBytes);

			// Подсчёт IP
			uint totalIps = prefix >= 32 ? 1u : (1u << (32 - prefix));
			uint availableHosts = prefix >= 31 ? 0u : totalIps - 2;


			// Первый и последний хост
			string firstHostAddress;
			string lastHostAddress;
			if (availableHosts > 0)
			{
				uint firstHostUint = netUint + 1;
				uint lastHostUint = broadcastUint - 1;
				byte[] firstHostBytes = BitConverter.GetBytes(firstHostUint).Reverse().ToArray();
				byte[] lastHostBytes = BitConverter.GetBytes(lastHostUint).Reverse().ToArray();
				firstHostAddress = string.Join(".", firstHostBytes);
				lastHostAddress = string.Join(".", lastHostBytes);
			}
			else
			{
				firstHostAddress = networkAddress;
				lastHostAddress = broadcastAddress;
			}

			// Установка значений
			textBoxNetwork.Text = networkAddress;
			textBoxBroadcast.Text = broadcastAddress;
			textBoxTotalIPs.Text = totalIps.ToString();
			textBoxAvailableHosts.Text = availableHosts.ToString();

			textBoxFirstHost.Text = firstHostAddress;
			textBoxLastHost.Text = lastHostAddress;

			Console.WriteLine("<---------------");
			Console.WriteLine($"Сеть: {networkAddress}");
			Console.WriteLine($"Broadcast: {broadcastAddress}");
			Console.WriteLine($"Всего IP: {totalIps}");
			Console.WriteLine($"Доступно хостов: {availableHosts}");
			Console.WriteLine("--------------->");
		}
		protected override void OnClosed(EventArgs e)
		{
			FreeConsole();
			base.OnClosed(e);
		}

	}
}
